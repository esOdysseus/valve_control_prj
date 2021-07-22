#include <CScheduler.h>
#include <Common.h>
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

void CScheduler::send_command( const std::string& peer_app, const std::string& peer_pvd, 
                               const std::string& json_data ) {
    try {
        unsigned long msg_id = 0;
        alias::CAlias peer(peer_app, peer_pvd);
        LOGI("Send request message to peer(%s/%s).", peer.app_path.data(), peer.pvd_id.data());

        {
            // we need lock for NOW-DB consistency-timing.

            // Trig peer to do activity according to a json-data. (send json-data to peer)
            //      We will get msg-id from MCommunicator->request()
            //      If msg-id == 0, then try again sending it with random wait. (Max retry 3 times.)
            msg_id = _m_comm_mng_->request( peer, json_data );

            // If get msg-id != 0, then append record to DataBase(Now-DB) with state == TRIGGERED & msg-id.
            // But msg-id == 0, then append record to DataBase(Now-DB) with state == FAIL & msg-id.

            // we need unlock for NOW-DB consistency-timing.
        }

        // Remove a record that is sent to peer from DataBase(Future-DB).
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
            if( rcmd.get() == NULL ) {
                throw std::runtime_error("pop_cmd() is invalid operation.");
            }

            std::string app_path = rcmd->who().get_app();
            std::string pvd_id = rcmd->who().get_pvd();
            auto when = rcmd->when();
            std::string t_when = when.get_type();

            // Display 6-principle
            LOGD("who=%s/%s", app_path.data(), pvd_id.data());
            LOGD("when=%s", t_when.data());
            LOGD("where=%s", rcmd->where().get_type().data());
            LOGD("what=%s_%d", rcmd->what().get_type().data(), rcmd->what().get_which());
            LOGD("how=%s", rcmd->how().get_method().data());

            // Check whether CMD is completed Task-Pair case, or not.
            // If CMD is in-completed Task-Pair case, then throw Exception.
            ;   // TODO

            // If "when" is relative-time or absolute-time is under now + 5 seconds, (Not "period when")
            // Then trigger peer to do activity by the CMD immediatlly.
            if( t_when == principle::CWhen::TYPE_ONECE || t_when == principle::CWhen::TYPE_SPECIAL_TIME ) {
                double cur_time = time_pkg::CTime::get<double>();

                if( when.get_start_time() <= (cur_time + 5.0) ) {
                    send_command( app_path, pvd_id, rcmd->get_payload() );
                    continue;
                }
            }

            // Classfy which When-info of CMD is EventBase-type or PeriodBase-type.
            // Store json-data of body in CMD to Database(Future-DB) according to type-info of "when" in CMD.
            if( t_when == principle::CWhen::TYPE_ROUTINE_DAY || t_when == principle::CWhen::TYPE_ROUTINE_WEEK ) {
                _m_db_.insert_record(Tdb::Ttype::ENUM_FUTURE, Tdb::DB_TABLE_PERIOD, rcmd);
            }
            else {
                _m_db_.insert_record(Tdb::Ttype::ENUM_FUTURE, Tdb::DB_TABLE_EVENT, rcmd);
            }
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
            Tdb::TVrecord records;
            Tdb& db_ref = _m_db_;
            double cur_time = time_pkg::CTime::get<double>();
            Tdb::TFPcond lamda_make_condition = [&cur_time](std::string kwho, std::string kwhen, 
                                                    std::string kwhere, std::string kwhat, 
                                                    std::string khow, std::string kuuid) -> std::string {
                // Load records from DataBase(Future-DB) if "when" is under now + 5 seconds.
                return (kwhen + " <= " + std::to_string(cur_time + 5.0) + " ORDER BY " + kwhen + " ASC");
            };
            Tdb::TFPconvert lamda_convertor = [&db_ref](Tdb::Ttype db_type, Tdb::Trecord& record) mutable -> void {
                // When we load json-data from PeriodBase tables, We must convert "period when" to "specific when".
                db_ref.convert_record_to_event(db_type, record);
            };

            // We have to load json-data per tables. (EventBase/PeriodBase)
            _m_db_.get_records(Tdb::Ttype::ENUM_FUTURE, Tdb::DB_TABLE_EVENT,  lamda_make_condition, nullptr, records );
            _m_db_.get_records(Tdb::Ttype::ENUM_FUTURE, Tdb::DB_TABLE_PERIOD, lamda_make_condition, lamda_convertor, records );

            // send command-msg to peer.
            for( auto itr=records.begin(); itr!=records.end(); itr++ ) {
                std::shared_ptr<Tdb::Trecord> record = *itr;
                auto peer = Tdb::get_who(*record);
                auto payload = Tdb::get_payload(*record);
                
                send_command(peer->app_path, peer->pvd_id, payload);
            }

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