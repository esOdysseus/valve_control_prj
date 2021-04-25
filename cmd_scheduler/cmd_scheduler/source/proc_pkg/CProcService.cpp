#include <cassert>
#include <unistd.h>

#include <logger.h>
#include <CProcService.h>
#include <CProcState.h>

namespace proc_pkg {


static double abs_double(double value) {
    return value < 0.0 ? (-1)*value : value;
}

/***********************************
 * Definition of Public Function
 */
CProcService::CProcService(void)
: IProc(SELF_NAME) { }

CProcService::~CProcService(void) { }

bool CProcService::is_available_service(std::string app_name) {
    try {
        if ( _monitor_.is_there(app_name) == true ) {
            MNT::ErrType state_err = _monitor_.get(app_name, MNT_ERROR::EMNT_ERROR_STATE);
            if ( state_err != 0 ) {
                LOGW( "%s is connected. but there is State-Error(%d).", app_name.c_str(), state_err );
                return false;
            }

            if ( (bool)(_monitor_.get(app_name, MNT_FLAG::EMNT_FLAG_SVC_AVAILABLE)) == true ) {
                return true;
            }
        }
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}

std::shared_ptr<CProcService::PacketType> CProcService::make_packet_for_dbgcmd(std::shared_ptr<CMDDebug> &cmd) {
    std::shared_ptr<PacketType> packet;
    LOGERR("Not Supported Function. Please check it.");
    return packet;
}

void CProcService::register_sent_msg(uint32_t msg_id, bool result) {
    LOGERR("Not Supported Function. Please check it.");
}

/***********************************
 * Definition of Protected Function
 */
bool CProcService::data_update(TaskType type, std::shared_ptr<CMDType> cmd) {
    LOGD("Called.");
    std::string target_app = cmd->get_from();
    std::shared_ptr<MNT::DataType> dumy;
    assert( (bool)(cmd->get_flag(E_FLAG::E_FLAG_KEEPALIVE)) == true );
    
    try {
        // validation check & if need, then trig time-updating.
        if ( validation_check(cmd) != true ) {
            LOGERR("Validation checking is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_CHECKING_CMD_VALIDATION);
        }

        // Creating Dest-App element as empty-component.
        if ( _monitor_.is_there(target_app) == false ) {
            if ( _monitor_.insert(target_app, dumy) != true ) {
                LOGW("Creating of Service-available checker is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_INSERT_DATA);
            }
        }

        // Set available-flag
        if ( _monitor_.update(target_app, MNT_FLAG::EMNT_FLAG_SVC_AVAILABLE, 1) != true ) {
            LOGW("Updating of available-flag is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
        }

        // Set error-flag
        if ( (bool)(cmd->get_flag(E_FLAG::E_FLAG_STATE_ERROR)) == true ) {
            if ( _monitor_.update(target_app, MNT_ERROR::EMNT_ERROR_STATE, cmd->get_state()) != true ) {
                LOGW("Updating of error-flag is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
            }
        }

        return true;
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}

bool CProcService::create_custom_threads(void) {
    LOGD("Called.");
    assert( _is_continue_ == true );

    this->_runner_period_ = std::thread(&CProcService::run_period, this);
    if ( this->_runner_period_.joinable() == false ) {
        LOGW("run_period thread creating is failed.");
        throw CException(E_ERROR::E_ERR_FAIL_CREATING_THREAD);
    }
    _is_custom_thread_ = true;
    LOGD("Done.");
    return _is_custom_thread_;
}

void CProcService::destroy_custom_threads(void) {
    LOGD("Called.");
    assert( _is_continue_ == false );

    _is_custom_thread_ = false;
    if(_runner_period_.joinable() == true) {
        _runner_period_.join();
    }
    LOGD("Done.");
}

/***********************************
 * Definition of Private Function
 */
int CProcService::run_period(void) {        // Thread: Period-running routin.
    std::list<MNT::IDType>::iterator itor;
    std::list<MNT::IDType> app_list;
    double elapsed_time = -1.0;

    // get ids of applications if consider disconnect
    MNT::FuncType lamda_read_only = [&](MNT::IDType id, double elapse_time)-> bool {
        if( elapse_time >= MAX_DELAY_FOR_DISCONNECT ) {
            app_list.push_back(id);
        }
        return true;
    };

    // If last Keep-Alive-time is very long ago, then ragard it is disconnected.
    // So remove it from CSvcMonitor.
    while(_is_continue_) {
        try{
            // run lamda function.
            app_list.clear();
            _monitor_.process_with_elapse_time(lamda_read_only);

            // remove CSvcMDs from CSvcMonitor according to id-list.
            itor = app_list.begin();
            while(itor != app_list.end()) {
                LOGW("CSvcMD(%s) data is removed from Monitor.", (*itor).c_str());
                assert(_monitor_.remove(*itor) == true);
                itor++;
            }
            LOGD("CSvcMD count[%d] in Monitor.", _monitor_.size());

            // TODO trig updating of CProcState.

            // clear & sleep.
            app_list.clear();
            sleep(1);       // period : 1 second
        }
        catch ( const std::exception &e ) {
            LOGERR("%s", e.what());
        }
    }

    return 0;
}

bool CProcService::validation_check(std::shared_ptr<CMDType> cmd) {
    IProc* proc_inf = NULL;
    double rcv_time = 0.0;
    double send_time = 0.0;

    try {
        // check time-synchronization between self-dev & valve-dev.
        assert( (rcv_time = cmd->get_rcv_packet_time()) > 0.0 );
        assert( (send_time = cmd->get_when()) > 0.0 );

        if ( abs_double(rcv_time - send_time) > 1.0 ) {
            // trig time-update.
            if( (proc_inf = find_proc(CProcState::SELF_NAME)) == NULL ) {
                LOGW("Not exist CProcState process.");
                throw CException(E_ERROR::E_ERR_NOT_EXISTED_PROC);
            }

            LOGW("Insert CMD for Time-Updating task.");
            assert( proc_inf->insert_cmd(cmd, TaskType::E_PROC_TASK_TIME_UPDATE) == true );
            throw CException(E_ERROR::E_ERR_NEED_TIME_SYNC);
        }

        return true;
    }
    catch( const std::exception &e ) {
        auto except = dynamic_cast<const CException&>(e);
        if( except.get() != E_ERROR::E_ERR_NEED_TIME_SYNC ) {
            LOGERR("%s", e.what());
        }
    }

    return false;
}



}