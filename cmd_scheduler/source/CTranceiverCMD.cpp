#include <cassert>
#include <iostream>

#include <stdio.h>

#include <logger.h>
#include <unistd.h>
#include <CTranceiverCMD.h>
#include <IProtocolInf.h>
#include <CCmd.h>
#include <IProcShare.h>
#include <MData.h>
#include <CException.h>

using namespace std;

namespace cmd_pkg {

/*********************************
 * Definition of Public Function.
 */
std::shared_ptr<CTranceiverCMD> CTranceiverCMD::get_instance(CommHandler *handler) {
    static std::shared_ptr<CTranceiverCMD> singleton;
    
    if( singleton.get() == NULL ) {
        assert(handler != NULL);
        singleton = std::shared_ptr<CTranceiverCMD>( new CTranceiverCMD(*handler) );
    }
    assert( singleton.get() != NULL );
    return singleton;
}

CTranceiverCMD::CTranceiverCMD(CTranceiverCMD::CommHandler handler){
    LOGD("Called.");
    std::string myself_name;
    std::shared_ptr<proc_pkg::IProcShare> procs_map;

    clear();
    _is_exit_ = false;

    // Initial-setting of variables.
    _h_communicator_ = std::forward<CommHandler>(handler);
    myself_name = _h_communicator_->get_app_id();

    // create proc-sharing-mapper & register it to all-of-procs.
    procs_map = std::make_shared<proc_pkg::IProcShare>();
    assert( _proc_state_.init(myself_name, procs_map) == true );
    assert( _proc_svc_.init(myself_name, procs_map) == true );
    assert( _proc_valve_.init(myself_name, procs_map) == true );
}

CTranceiverCMD::~CTranceiverCMD(void) {
    LOGD("Called.");
    exit();
}

void CTranceiverCMD::exit(void) {
    if (_is_exit_ == false) {
        _is_exit_ = true;
        _h_communicator_->quit();   // quit terminate.
        destroy_threads();          // exit threads
    }
    clear();                        // clear variables
}

// This-APP was normal-ready/quit to communicate between VMs.
void CTranceiverCMD::cb_initialization(enum_c::ProviderType provider_type, bool flag_init) {
    LOGD("Called. arg(flag_init=%d)", flag_init);

    try {
        if( _is_continue_ == false) {
            _is_continue_ = create_threads();
        }

        if( _is_continue_ == false ) {
            destroy_threads();
            throw CException(E_ERR_FAIL_CREATING_THREAD);
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }
}

// This-APP was exit from communication-session, abnormally.
void CTranceiverCMD::cb_abnormally_quit(const std::exception &e) {
    LOGW("Called. arg(error=%s)", e.what());
}

// Client was connected.
void CTranceiverCMD::cb_connected(std::string client_id, bool flag_connect) {
    LOGD("Called. arg(flag_connect=%d)", flag_connect);
}

// We receved a message from client_id.
void CTranceiverCMD::cb_receive_msg_handle(std::string client_id, std::shared_ptr<payload::CPayload> payload) {
    LOGD("Called.");

    size_t data_size = 0;
    std::string cmd_type;
    std::shared_ptr<CMDDebug> dbg_cmd;
    std::shared_ptr<CMDType> cmd;
    const void* data = payload->get_payload(data_size);

    try {
        if( _is_exit_ == false ) {
            cout << "************************************" << endl;
            cout << "* 1. Client-ID : " << client_id << endl;
            cout << "* 2. CPayload-Name : " << payload->get_name() << endl;
            cout << "* 3. payload-size : " << data_size << endl;
            cout << "* 4. payload : " << (const char*)data << endl;
            cout << "************************************" << endl;
            
            // Parsing of received-CMD.
            cmd = std::make_shared<CMDType>(client_id);
            cmd_type = cmd->get_sof(data, (ssize_t)data_size);
            if (cmd_type.empty() == true) {
                throw CException(E_ERROR::E_NO_ERROR);
            }

            if ( cmd_type == CMD_UNIVERSE ) {
                // Decode Universal-Command(UCMD).
                if ( cmd->decode(data, (ssize_t)data_size) != true ) {
                    LOGERR("Decoding of Universal-Command is failed. Please check it.");
                    throw CException(E_ERROR::E_ERR_FAIL_DECODING_CMD);
                }

                // Trig thread for updating data according to received CMD.
                if ( update_trig(cmd) != true ) {
                    LOGW("It's failed to trig updating data.");
                    throw CException(E_ERROR::E_ERR_FAIL_TRIG_UPDATE_DATA);
                }
            }
            else if ( cmd_type == CMD_DEBUG ) {
                // Decode Debug-Command
                dbg_cmd = std::make_shared<CMDDebug>(client_id, 
                                                    data, 
                                                    (ssize_t)data_size);
                assert( dbg_cmd.get() != NULL );

                if ( insert_new_cmd(dbg_cmd) != true ) {
                    LOGW("It's failed to insert CMD to list.");
                    throw CException(E_ERROR::E_ERR_FAIL_INSERT_CMD);
                }
            }
        }
    }
    catch(const CException &e) {
        if ( e.get() != E_ERROR::E_NO_ERROR ) {
            LOGERR("%s", e.what());
        }
    }
    catch(const std::exception &e){
        LOGERR("%s", e.what());
    }

}

bool CTranceiverCMD::insert_reloaded_cmd(std::shared_ptr<CMDDebug> &cmd) {
    auto instance = CTranceiverCMD::get_instance();
    assert( instance.get() != NULL );
    return instance->insert_new_cmd(cmd);
}

/************************************
 * Definition of Private-Function.
 */
void CTranceiverCMD::clear(void) {
    _is_continue_ = false;
    _h_communicator_.reset();
    _cmd_list_.clear();
}

bool CTranceiverCMD::create_threads(void) {
    // create local threads.
    _is_continue_ = true;

    this->_runner_exe_cmd_ = std::thread(&CTranceiverCMD::run_cmd_execute, this);
    if ( this->_runner_exe_cmd_.joinable() == false ) {
        LOGW("run_cmd_execute thread creating is failed.");
        _is_continue_ = false;
    }

    return _is_continue_;
}

void CTranceiverCMD::destroy_threads(void) {
    // destroy local threads.
    _is_continue_ = false;
    _wake_exe_cmd_.notify_all();

    if(_runner_exe_cmd_.joinable() == true) {
        _runner_exe_cmd_.join();
    }
    
    // destroy proc_pkg threads.
    _proc_state_.destroy_threads();
    _proc_svc_.destroy_threads();
    _proc_valve_.destroy_threads();
}

bool CTranceiverCMD::update_trig(std::shared_ptr<CMDType> &cmd) {
    CMDType::FlagType flag = 0;

    // Keep-Alive message
    if ( (bool)(cmd->get_flag(E_FLAG::E_FLAG_KEEPALIVE)) == true ) {
        LOGI("UCMD KEEP-ALIVE message.");
        // If expire time, then check out-of-service about that.
        if( _proc_svc_.insert_cmd(cmd) != true ) {
            LOGW("Inserting Keep-Alive MSG is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_INSERT_CMD);
        }
    }
    // ACK or ACTion-Done message
    else if ( (bool)(flag=cmd->get_flag(E_FLAG::E_FLAG_ACK_MSG | 
                                        E_FLAG::E_FLAG_ACTION_DONE)) == true ) {
        LOGI("UCMD ACK(%d) or ACT-DONE(%d) message : %d", E_FLAG::E_FLAG_ACK_MSG, E_FLAG::E_FLAG_ACTION_DONE, flag);
        // If expire time , then re-transfer message. (default limit 3 times)
        if( _proc_valve_.insert_cmd(cmd) != true ) {
            LOGW("Inserting ACK/ACT-DONE MSG is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_INSERT_CMD);
        }
    }
    // Normal message (REQ/RESP/PUB message)
    else {
        LOGI("UCMD Normal message.");
        // Apply time to local-time in this system.
        if( _proc_valve_.insert_cmd(cmd) != true ) {
            LOGW("Inserting REQ/RESP/PUB MSG is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_INSERT_CMD);
        }
    }
    return true;
}

bool CTranceiverCMD::insert_new_cmd(std::shared_ptr<CMDDebug> cmd) {
    assert( cmd.get() != NULL );
    assert( cmd->is_there() == true );

    try {
        if ( _is_continue_ ) {
            std::unique_lock<std::mutex> lock(_mtx_cmd_list_);
            _cmd_list_.push_back(cmd);
            _wake_exe_cmd_.notify_one();
            return true;
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
    
    return false;
}

/********************************
 * Definition of Thread-Routin
 */
int CTranceiverCMD::run_cmd_execute(void) {
    LOGD("Called.");
    std::shared_ptr<CMDDebug> cmd;
    auto func_decide_wakeup_time = [&](void)->bool{ 
        if ( _cmd_list_.size() > 0 || _is_continue_ == false )
            return true;
        return false;
    };

    std::unique_lock<std::mutex> lock(_mtx_cmd_list_);
    while(_is_continue_) {
        _wake_exe_cmd_.wait(lock, func_decide_wakeup_time );
        while( _cmd_list_.size() ) {
            cmd.reset();
            cmd = *(_cmd_list_.begin());
            _cmd_list_.pop_front();
            lock.unlock();

            try {   // execute command
                if ( execute_cmd(cmd) != true ) {
                    LOGW("Execution of Command is failed.");
                    throw CException(E_ERROR::E_ERR_FAIL_EXECUTE_CMD);
                }
            }
            catch (const CException &e) {
                if ( e.get() != E_ERROR::E_NO_ERROR )
                    LOGERR("%s", e.what());
            }
            catch (const std::exception &e) {
                LOGERR("%s", e.what());
            }

            lock.lock();
        }
    }
}

bool CTranceiverCMD::execute_cmd(std::shared_ptr<CMDDebug> &cmd) {
    bool sent_res = false;
    std::shared_ptr<monitor_pkg::Cmsg_packet> packet;
    std::string command = cmd->get_cmd();
    auto lamda_make_packet = [&]() {
        // Check Command & make packet
        if ( command == CMD_STATE ) {
            LOGD("execute \"%s\" CMD", CMD_STATE);
            packet = _proc_state_.make_packet_for_dbgcmd(cmd);
        }
        // else if( command == CMD_SERVICES ) {
        //     LOGD("execute \"%s\" CMD", CMD_SERVICES);
        //     packet = _proc_svc_.make_packet_for_dbgcmd(cmd);
        // }
        else if( command == CMD_VCONTROL ) {
            LOGD("execute \"%s\" CMD", CMD_VCONTROL);
            packet = _proc_valve_.make_packet_for_dbgcmd(cmd);
        }
        else {
            LOGW("Not Supported Command.(%s)", command.c_str());
            throw CException(E_ERROR::E_ERR_NOT_SUPPORTED_CMD);
        }

        // Check packet for validation
        if (packet.get() == NULL) {
            LOGW("Making packet is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_MAKING_PACKET);
        }
    };


    try {   // Routin is Start
        if ( cmd->is_there() == false ) {
            LOGW("Decoding of Debug-Command is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_DECODING_CMD);
        }
        
        // Pack packet
        lamda_make_packet();
        
        // Send packet
        sent_res = send(packet->request_to, packet->data, packet->data_size);
        packet->proc_pointer->register_sent_msg(packet->msg_id, sent_res);
        if ( sent_res != true ) {
            LOGW("Sending is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_SENDING_PACKET);
        }

        return true;
    }
    catch( const CException &e ) {
        if (e.get() == E_ERROR::E_NO_ERROR )
            throw;
        LOGERR("%s", e.what());
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}

bool CTranceiverCMD::send(std::string who, const void *data, ssize_t data_size) {
    std::shared_ptr<payload::CPayload> new_payload = _h_communicator_->create_payload();
    assert(data != NULL);
    assert(data_size > 0);

    try {
        new_payload->set_payload(data, data_size);
        if(_h_communicator_->send(who, new_payload) != true) {
            LOGW("Sending packet is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_SENDING_PACKET);
        }
        return true;
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
    }

    return false;
}


}   // namespace cmd_pkg