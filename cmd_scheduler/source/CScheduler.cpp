#include <CScheduler.h>

#include <logger.h>

using namespace std::placeholders;

namespace service {


const std::string CScheduler::APP_PATH = "CMD-Scheduler";
const std::string CScheduler::PVD_COMMANDER = "cmd_tranceiver";
const std::string CScheduler::PVD_DEBUGGER = "def_debugger";


/*********************************
 * Definition of Public Function.
 */
std::shared_ptr<CScheduler> CScheduler::get_instance( void ) {
    static std::shared_ptr<CScheduler> _instance_( new CScheduler() );
    return _instance_;
}

bool CScheduler::init( std::string file_path_alias, std::string file_path_proto ) {
    try {
        const comm::MCommunicator::TProtoMapper mapper = {
            { PVD_COMMANDER, file_path_proto },
            { PVD_DEBUGGER, std::string() }
        };
        
        _m_comm_mng_ = std::make_shared<comm::MCommunicator>( APP_PATH, file_path_alias, mapper );
        if( _m_comm_mng_.get() == NULL ) {
            throw std::runtime_error("MCommunicator mem-allocation is failed.");
        }

        _m_comm_mng_->register_listener( PVD_COMMANDER, std::bind(&CScheduler::receive_command, this, _1) );
        _m_comm_mng_->register_listener( PVD_DEBUGGER, std::bind(&CScheduler::receive_command, this, _1) );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CScheduler::start( void ) {
    if( _m_comm_mng_.get() == NULL ) {
        throw std::runtime_error("MCommunicator is NULL.");
    }

    create_threads();
    _m_comm_mng_->start();
}

void CScheduler::exit( void ) {
    destroy_threads();
    clear();
}

CScheduler::~CScheduler(void) {
    exit();
    LOGD("Terminated.");
}


/*********************************
 * Definition of Private Function.
 */
CScheduler::CScheduler( void ) {
    clear();
}

void CScheduler::clear( void ) {
    _m_comm_mng_.reset();
    _m_is_continue_ = false;

    {
        std::unique_lock<std::mutex> lk(_mtx_queue_lock_);
        while( _mv_cmds_.empty() == false ) {
            _mv_cmds_.pop();
        }
    }
}

void CScheduler::receive_command( std::shared_ptr<cmd::ICommand>& cmd ) {
    try {
        LOGD("Enter");
        if( cmd.get() == NULL ) {
            throw std::invalid_argument("Invalid CMD is NULL.");
        }

        // Invalid CMD checking
        if( cmd->is_parsed() == false ) {
            throw std::invalid_argument("CMD is not decoded.");
        }

        push_cmd( cmd );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/** Push function for Blocking queue. */
void CScheduler::push_cmd( std::shared_ptr<cmd::ICommand>& cmd ) {
    try {
        std::unique_lock<std::mutex> lk(_mtx_queue_lock_);

        if (false == _m_is_continue_.load()) {
            std::string err = "Thread termination is occured.";
            throw std::out_of_range(err);
        }

        _mv_cmds_.emplace( cmd );
        _m_queue_cv_.notify_all();
    }
    catch ( const std::out_of_range& e ) {
        LOGW("%s", e.what());
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/** Pop function for Blocking queue. */
std::shared_ptr<cmd::ICommand> CScheduler::pop_cmd( void ) {    // Blocking 
    std::shared_ptr<cmd::ICommand> cmd;
    try {
        std::unique_lock<std::mutex> lk(_mtx_queue_lock_);

        if ( (true == _mv_cmds_.empty()) && (true == _m_is_continue_.load()) ) {
            _m_queue_cv_.wait(lk, [&]() {
                return ((false == _mv_cmds_.empty()) || (false == _m_is_continue_.load()));
            });
        }

        if (false == _m_is_continue_.load()) {
            std::string err = "Thread termination is occured.";
            throw std::out_of_range(err);
        }

        if( _mv_cmds_.empty() == true ) {
            std::string err = "CMDs-queue is empty.";
            throw std::logic_error(err);
        }

        cmd = _mv_cmds_.front();
        _mv_cmds_.pop();
    }
    catch ( const std::out_of_range& e ) {
        LOGW("%s", e.what());
        throw e;
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return cmd;
}


/****
 * Thread related functions
 */
void CScheduler::create_threads(void) {
    try {
        if( _m_is_continue_.exchange(true) == false ) {
            LOGI("Create RX-cmd handle-thread.");
            _mt_rcmd_handler_ = std::thread(&CScheduler::handle_rx_cmd, this);

            LOGI("Create TX-cmd handle-thread.");
            _mt_scmd_handler_ = std::thread(&CScheduler::handle_tx_cmd, this);

            if ( _mt_rcmd_handler_.joinable() == false ) {
                _m_is_continue_ = false;
            }

            if ( _mt_scmd_handler_.joinable() == false ) {
                _m_is_continue_ = false;
            }
        }

        if( _m_is_continue_ == false ) {
            destroy_threads();
            throw std::runtime_error("Creating Tx/Rx-CMD handle-threads are failed.");
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CScheduler::destroy_threads(void) {
    if( _m_is_continue_.exchange(false) == true ) {
        if( _mt_rcmd_handler_.joinable() == true ) {
            LOGI("Destroy RX-cmd handle-thread.");     // Destroy of RX-cmd handle-thread.
            _mt_rcmd_handler_.join();
        }

        if( _mt_scmd_handler_.joinable() == true ) {
            LOGI("Destroy TX-cmd handle-thread.");     // Destroy of TX-cmd handle-thread.
            _mt_scmd_handler_.join();
        }
    }
}

/****
 * Treading for RX/TX Command
 */
int CScheduler::handle_rx_cmd(void) {
    while(_m_is_continue_.load()) {
        try {
            auto rcmd = pop_cmd();      // Blocking 

            ;   // TODO Display 6-principle & Convert Task-Pair(Open/Close) base on Absolute-Time.
            ;   // TODO Store Task-Pair for coresponding to One-CMD.

        }
        catch (const std::exception &e) {
            LOGERR("%s", e.what());
        }
    }

    LOGI("Exit RX-cmd handle-thread.");
    return 0;
}

int CScheduler::handle_tx_cmd(void) {
    while(_m_is_continue_.load()) {
        try {
            ;   // TODO If needed, load Task-Pair from Stored-CMDs for coresponding to One-CMD.
            ;   // TODO Make a CMD it's consist of Task-Pair(Open/Close) base on Absolute-Time.

            // wait 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        catch (const std::exception &e) {
            LOGERR("%s", e.what());
        }
    }

    LOGI("Exit TX-cmd handle-thread.");
    return 0;
}


}   // service