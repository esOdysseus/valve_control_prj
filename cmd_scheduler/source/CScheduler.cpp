#include <CScheduler.h>
#include <ICommand.h>

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
                               std::shared_ptr<Tdb::Trecord>& record ) {
    try {
        alias::CAlias peer(peer_app, peer_pvd);
        send_command( peer, record );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CScheduler::send_command( alias::CAlias& peer, std::shared_ptr<Tdb::Trecord>& record ) {
    try {
        uint32_t msg_id = 0;
        Tdb::Tstate state = Tdb::Tstate::ENUM_FAIL;
        Tdb::Ttype db_type = Tdb::Ttype::ENUM_PAST;
        std::string payload = Tdb::get_payload(*record);
        LOGI("Send request message to peer(%s/%s).", peer.app_path.data(), peer.pvd_id.data());

        // we need lock for NOW-DB consistency-timing.
        std::lock_guard<std::mutex> locker(_mtx_send_lock_);

        // Trig peer to do activity according to a json-data. (send json-data to peer)
        //      We will get msg-id from MCommunicator->request()
        msg_id = _m_comm_mng_->request( peer, payload, common::E_STATE::E_STATE_THR_CMD );

        // If get msg-id != 0, then append record to DataBase(Now-DB) with state == TRIGGERED & msg-id.
        // But msg-id == 0, then append record to DataBase(PAST-DB) with state == FAIL & msg-id.
        if( msg_id != 0 ) {
            state = Tdb::Tstate::ENUM_TRIG;
            db_type = Tdb::Ttype::ENUM_NOW;
        }

        _m_db_.append(record, Tdb::Tkey::ENUM_MSG_ID, msg_id);
        _m_db_.append(record, Tdb::Tkey::ENUM_STATE, state);
        _m_db_.insert_record(db_type, Tdb::DB_TABLE_EVENT, record);
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

double CScheduler::convert_json_to_event( std::string& payload, double& next_when ) {
    double when = 0.0;
    try {
        if( payload.empty() == true ) {
            throw std::logic_error("payload is empty. we need it.");
        }

        auto json = std::make_shared<json_mng::CMjson>();
        LOGD("Before: payload=%s , length=%u", payload.c_str(), payload.length());
        if( json->parse(payload.c_str(), payload.length()) != true ) {
            throw std::runtime_error("Json Parsing is failed.");
        }

        // check version.
        auto ver = cmd::ICommand::extract_version(json);
        LOGI("CMD Version = %s", ver.data());

        // parse "when" part in principle-6.
        auto cwhen = cmd::ICommand::extract_when(json);
        LOGD( "Success parse of Json buffer." );

        // make specific eventual 'when'-part.
        when = cwhen->get_start_time();
        next_when = cwhen->get_next_period(when);
        cwhen.reset();
        cwhen = std::make_shared<principle::CWhen>(principle::CWhen::TYPE_SPECIAL_TIME, when);
        if( cwhen.get() == NULL ) {
            throw std::runtime_error("'when' memory allocation is failed.");
        }

        // apply new event-'when' to JSON-payload.
        if( cmd::ICommand::apply_when(json, cwhen) == false ) {
            throw std::logic_error("appling 'when'-part to json is failed.");
        }

        payload.clear();
        payload = std::string( json->print_buf() );
        LOGD("After : payload=%s , length=%u", payload.c_str(), payload.length());
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return when;
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

bool CScheduler::process_now_space( std::shared_ptr<cmd::ICommand>& cmd ) {
    bool result = false;
    try {
        uint16_t cmd_state = 0;
        uint32_t msg_id = 0;
        Tdb::Tstate state;
        std::shared_ptr<cmd::CuCMD> ucmd;

        if( cmd->proto_name() != std::string(cmd::CuCMD::PROTOCOL_NAME) ) {
            std::string warn = "Protocol is not " + std::string(cmd::CuCMD::PROTOCOL_NAME);
            throw std::out_of_range(warn);
        }
        ucmd = std::dynamic_pointer_cast<cmd::CuCMD>( cmd );

        // If flag is ACK/ACT-START/STATE-ERROR/RESP message, then update NOW-DB setting.
        if( ucmd->get_flag( Eflag::E_FLAG_ACK_MSG | Eflag::E_FLAG_ACTION_START | 
                            Eflag::E_FLAG_STATE_ERROR | Eflag::E_FLAG_RESP_MSG ) == 0 ) {
            throw std::out_of_range("CMD-flag is not ACK/START/RESP/ERROR msg.");
        }

        // Check Peer System-Error.
        cmd_state = ucmd->get_state();
        if( ucmd->get_flag(Eflag::E_FLAG_STATE_ERROR) && (cmd_state & Estate::E_STATE_ACTION_FAIL) == 0 ) {
            std::string warn = "Peer(" + ucmd->get_from().app_path + "/" + ucmd->get_from().pvd_id + ") has some system-error.";
            throw std::invalid_argument(warn);
        }

        // Get Message-ID
        msg_id = ucmd->get_id();
        if( msg_id == 0 ) {
            throw std::invalid_argument("msg_id is NULL. (invalid CMD)");
        }

        Tdb::TFPcond lamda_make_condition = [&msg_id](std::string kwho, std::string kwhen, 
                                                      std::string kwhere, std::string kwhat, 
                                                      std::string khow, std::string kuuid,
                                                      std::map<Tdb::Tkey, std::string>& kopt) -> std::string {
            // Load records from DataBase(NOW-DB) if "msg_id" is equal with targeting msg-id.
            auto key_msgid = kopt[Tdb::Tkey::ENUM_MSG_ID];
            return (key_msgid + " == " + std::to_string(msg_id) + " ORDER BY " + kwhen + " DESC");
        };
        
        // Get State value.
        if( ucmd->get_flag(Eflag::E_FLAG_ACK_MSG) ) {
            state = Tdb::Tstate::ENUM_RCV_ACK;
        }
        else if ( ucmd->get_flag(Eflag::E_FLAG_ACTION_START) ) {
            state = Tdb::Tstate::ENUM_STARTED;
        }
        else if ( ucmd->get_flag(Eflag::E_FLAG_STATE_ERROR) ) {
            state = Tdb::Tstate::ENUM_FAIL;
        }
        else if ( ucmd->get_flag(Eflag::E_FLAG_RESP_MSG) ) {
            state = Tdb::Tstate::ENUM_DONE;
        }

        // Update State in NOW-db.
        _m_db_.update_record(Tdb::Ttype::ENUM_NOW, Tdb::DB_TABLE_EVENT, 
                             Tdb::Tkey::ENUM_MSG_ID, msg_id, 
                             Tdb::Tkey::ENUM_STATE, state);

        // If Action is Done/Fail, then move record from NOW-db to PAST-db.
        if( state == Tdb::Tstate::ENUM_FAIL || state == Tdb::Tstate::ENUM_DONE ) {
            auto records = _m_db_.get_records(Tdb::Ttype::ENUM_NOW, Tdb::DB_TABLE_EVENT, lamda_make_condition, nullptr);
            auto itr = records->begin();
            if( itr == records->end() ) {
                std::string err = "Record(msg-id: " + std::to_string(msg_id) + ") is not exist in NOW-db.";
                throw std::out_of_range(err);
            }
            _m_db_.insert_record(Tdb::Ttype::ENUM_PAST, Tdb::DB_TABLE_EVENT, *itr);
            _m_db_.remove_record(Tdb::Ttype::ENUM_NOW, Tdb::DB_TABLE_EVENT, *itr);
        }
        result = true;
    }
    catch( const std::out_of_range& e ) {
        LOGI("%s", e.what());
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

bool CScheduler::process_my_command( std::shared_ptr<cmd::ICommand>& cmd ) {
    bool result = false;
    try {
        auto who = cmd->who();
        if( who.get_app() != APP_PATH ) {
            std::string info = "CMD is not self-APP_PATH(" + APP_PATH + ")";
            throw std::out_of_range(info);
        }

        ;   // TODO
        result = true;
    }
    catch( const std::out_of_range& e ) {
        LOGI("%s", e.what());
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

void CScheduler::process_future_space( std::shared_ptr<cmd::ICommand>& rcmd ) {
    try {
        std::string app_path = rcmd->who().get_app();
        std::string pvd_id = rcmd->who().get_pvd();
        auto when = rcmd->when();
        std::string t_when = when.get_type();

        // Display 6-principle
        LOGD("who=%s/%s", app_path.data(), pvd_id.data());
        LOGD("when=%s", t_when.data());
        LOGD("where=%s", rcmd->where().get_type().data());
        LOGD("what=%s", rcmd->what().get_type().data());
        LOGD("how=%s", rcmd->how().get_type().data());

        // Check whether CMD is completed Task-Pair case, or not.
        // If CMD is in-completed Task-Pair case, then throw Exception.
        ;   // TODO

        // If "when" is relative-time or absolute-time is under now + 5 seconds, (Not "period when")
        // Then trigger peer to do activity by the CMD immediatlly.
        if( t_when == principle::CWhen::TYPE_ONECE || t_when == principle::CWhen::TYPE_SPECIAL_TIME ) {
            double cur_time = time_pkg::CTime::get<double>();

            if( when.get_start_time() <= (cur_time + 5.0) ) {
                auto record = _m_db_.make_base_record(rcmd);
                send_command( app_path, pvd_id, record );
                return ;
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
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
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

            /** for Received ACK/Action-Start/Action-Fail/Resp(Done) */
            if( process_now_space( rcmd ) == true ) {
                continue;
            }

            /** for Received Request-CMD for me(who) */
            if( process_my_command( rcmd ) == true ) {
                continue;
            }

            /** for Received Request-CMD for peer(who) */
            process_future_space( rcmd );
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
            Tdb& db_ref = _m_db_;
            double cur_time = time_pkg::CTime::get<double>();
            Tdb::TFPcond lamda_make_condition = [&cur_time](std::string kwho, std::string kwhen, 
                                                    std::string kwhere, std::string kwhat, 
                                                    std::string khow, std::string kuuid,
                                                    std::map<Tdb::Tkey, std::string>& kopt) -> std::string {
                // Load records from DataBase(Future-DB) if "when" is under now + 5 seconds.
                return (kwhen + " <= " + std::to_string(cur_time + 5.0) + " ORDER BY " + kwhen + " ASC");
            };
            Tdb::TFPconvert lamda_convertor = [&](Tdb::Ttype db_type, Tdb::Trecord& record, std::string& payload) -> void {
                // When we load json-data from PeriodBase tables, We must convert "period when" to "specific when".
                double when = 0.0;
                double next_when = 0.0;
                std::string legacy_uuid = db_ref.get_uuid(record);

                when = convert_json_to_event( payload, next_when );
                db_ref.convert_record_to_event(db_type, record, when);
                db_ref.update_record(db_type, Tdb::DB_TABLE_PERIOD, 
                                     Tdb::Tkey::ENUM_UUID, legacy_uuid, 
                                     Tdb::Tkey::ENUM_WHEN, next_when);
            };

            // We have to load json-data per tables. (EventBase/PeriodBase)
            auto records = _m_db_.get_records(Tdb::Ttype::ENUM_FUTURE, Tdb::DB_TABLE_EVENT,  lamda_make_condition, nullptr );
            _m_db_.get_records(Tdb::Ttype::ENUM_FUTURE, Tdb::DB_TABLE_PERIOD, lamda_make_condition, lamda_convertor, records );

            // send command-msg to peer.
            for( auto itr=records->begin(); itr!=records->end(); itr++ ) {
                std::shared_ptr<Tdb::Trecord> record = *itr;
                auto peer = Tdb::get_who(*record);
                
                send_command(*peer, record);
                // remove record from Future-DB.  (Assumption: "get_records" about Future-DB is used only in this function.)
                _m_db_.remove_record(Tdb::Ttype::ENUM_FUTURE, Tdb::DB_TABLE_EVENT, Tdb::get_uuid(*record));
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