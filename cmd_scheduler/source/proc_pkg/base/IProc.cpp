
#include <logger.h>
#include <IProc.h>
#include <MData.h>
#include <CException.h>

namespace proc_pkg {


/*********************************
 * Definition of Public Function.
 */
IProc::IProc(std::string proc_name) {
    clear();
    _proc_name_ = proc_name;
}

IProc::~IProc(void) {
    destroy_threads();
    if ( _is_init_ ) {
        _procs_map_->remove(*this);
        _procs_map_.reset();
    }
    clear();
    _proc_name_.clear();
}

bool IProc::init(std::string &myself_name, std::shared_ptr<ShareProcsType> instance) {
    assert( instance.get() != NULL );
    assert( myself_name.empty() == false );

    try {
        _app_name_ = myself_name;
        _procs_map_ = instance;
        assert( _procs_map_->insert(*this) == true );
        _is_init_ = create_threads();
        create_custom_threads();
    }
    catch( const std::exception &e ){
        LOGERR("%s", e.what());
        clear();
    }

    return _is_init_;
}

std::string IProc::get_proc_name(void) {
    return _proc_name_;
}

void IProc::destroy_threads(void) {
    _is_continue_ = false;
    _wake_runner_.notify_all();

    if(_runner_update_.joinable() == true) {
        _runner_update_.join();
    }
    destroy_custom_threads();
}

template <>
bool IProc::insert_cmd(std::shared_ptr<CMDType> &cmd, TaskType type) {
    std::shared_ptr<UCMD_STType> new_data;
    assert( cmd.get() != NULL );
    assert( cmd->parsing_complet() == true );

    try {
        if ( _is_continue_ ) {
            new_data = std::make_shared<UCMD_STType>(cmd, type);

            std::unique_lock<std::mutex> lock(_mtx_cmd_list_);
            _cmd_list_.push_back(new_data);
            _wake_runner_.notify_one();
            return true;
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
    
    return false;
}

template <>
bool IProc::insert_cmd(std::shared_ptr<CMDDebug> &cmd, TaskType type) {
    std::shared_ptr<DCMD_STType> new_data;
    assert( cmd.get() != NULL );
    assert( cmd->is_there() == true );
    assert( _is_custom_thread_ == true );

    try {
        if ( _is_continue_ ) {
            new_data = std::make_shared<DCMD_STType>(cmd, type);

            std::unique_lock<std::mutex> lock(_mtx_dbgk_cmd_list_);
            _dbgk_cmd_list_.push_back(new_data);
            return true;
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
    
    return false;
}

/************************************
 * Definition of Protected Function.
 */
std::string IProc::get_app_name(void) {
    return _app_name_;
}

IProc* IProc::find_proc(std::string name) {
    assert(_is_init_ == true);

    if ( name == _proc_name_ ) {
        LOGW("Not Support find self-instance.");
        return NULL;
    }

    // find proc-instance.
    return _procs_map_->get(name);
}

bool IProc::create_custom_threads(void) {
    LOGW("[%s] Not Implemented yet.", _proc_name_.c_str());
    _is_custom_thread_ = false;
    return _is_custom_thread_;
}

void IProc::destroy_custom_threads(void) {
    LOGW("[%s] Not Implemented yet.", _proc_name_.c_str());
    assert( _is_custom_thread_ == false );
}

/*********************************
 * Definition of Private Function.
 */
bool IProc::create_threads(void) {
    _is_continue_ = true;

    this->_runner_update_ = std::thread(&IProc::run_update_data, this);
    if ( this->_runner_update_.joinable() == false ) {
        LOGW("run_cmd_execute thread creating is failed.");
        _is_continue_ = false;
    }

    return _is_continue_;
}

void IProc::clear(void) {
    _is_init_ = false;
    _app_name_.clear();
    _procs_map_.reset();
    _is_continue_ = false;
    _is_custom_thread_ = false;
    _cmd_list_.clear();
    _dbgk_cmd_list_.clear();
}

int IProc::run_update_data(void) { // Thread: Data Update-routin.
    LOGD("Called.");
    std::shared_ptr<UCMD_STType> data;
    auto func_decide_wakeup_time = [&](void)->bool { 
        if ( _cmd_list_.size() > 0 || _is_continue_ == false )
            return true;
        return false;
    };

    std::unique_lock<std::mutex> lock(_mtx_cmd_list_);
    while(_is_continue_) {
        _wake_runner_.wait(lock, func_decide_wakeup_time );
        while( _cmd_list_.size() ) {
            data.reset();
            data = *(_cmd_list_.begin());
            _cmd_list_.pop_front();
            lock.unlock();

            try {   // execute command
                if ( data_update(data->task_type, data->cmd) != true ) {
                    LOGW("Data-Updating is failed.");
                    throw CException(E_ERROR::E_ERR_FAIL_UPDATE_DATA);
                }
            }
            catch (const std::exception &e) {
                LOGERR("%s", e.what());
            }

            lock.lock();
        }
    }
}


} // namespace proc_pkg