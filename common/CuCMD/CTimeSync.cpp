#include <math.h>
#include <limits>

#include <logger.h>
#include <CuCMD/CTimeSync.h>
#include <time_kes.h>


/**********
 * Assumption
 *  1. SCH(cmd-scheduler) has to enable time with GPS-data(TIME_SRC).
 *      - If a process has sensitive data with regard to time-coordinate changing, 
 *        then the process must enable time with GPS-data(TIME_SRC) at booting-time.
 *  2. SCH is connected with other SCHs, directly.
 *  3. Reliable-Time peer is allowed within 2-Hop distance from a SCH.
 *      - If you want to extent the reliable-time scope, then you have to add peer that has GPS-data(TIME_SRC) within 2-Hop distance.
 *  4. All of peer is located in similar space(Korea) that is treated as equal time-coordinate system.
 *  5. Can be multiple TIME_SRC peers that it's running on equal Machine-dev, 
 *     But when update-time, we must apply with only one TIME_SRC for one Machine-dev.
 *  6. If I have TIME_ON state, then I have qualified to do notify-update to peers.
 *  7. When boot, update-time processing is trigged by first peer-connection.
 **********/
namespace cmd {

class CTimeSync::CServerInfo {
public:
    CServerInfo(std::string& app_path, std::string& pvd_id, std::shared_ptr<ICommunicator>& comm) {
        try {
            if( app_path.empty() == true || pvd_id.empty() == true || comm.get() == NULL ) {
                throw std::invalid_argument("There is invalid argument.");
            }

            _m_app_path_ = app_path;
            _m_pvd_id_ = pvd_id;
            _m_comm_ = comm;
        }
        catch( const std::exception& e ) {
            LOGERR("%s", e.what());
            throw e;
        }
    }

    ~CServerInfo(void) {
        _m_comm_.reset();
        _m_pvd_id_.clear();
        _m_app_path_.clear();
    }

    bool connect_try(void) {
        return _m_comm_->connect_try( std::forward<std::string>(_m_app_path_), std::forward<std::string>(_m_pvd_id_) );
    }

    void disconnect(void) {
        _m_comm_->disconnect( _m_app_path_, _m_pvd_id_ );
    }

    std::string name(void) {
        return _m_app_path_ + "/" + _m_pvd_id_;
    }

private:
    std::shared_ptr<ICommunicator> _m_comm_;

    std::string _m_app_path_;

    std::string _m_pvd_id_;

};


constexpr const double CTimeSync::TIME_TREATE_AS_DISCONNACT;
constexpr const int64_t CTimeSync::TIME_SEND_PERIOD_KEEPALIVE;
constexpr const int64_t CTimeSync::TIME_UPDATE_PERIOD;
constexpr const double CTimeSync::TIME_INTERVAL_THRESOLDER;


/**********************************
 * Definition of Public Function.
 */
CTimeSync::CTimeSync( std::shared_ptr<::alias::CAlias>& myself, TFsend func, const double holding_time_on, const char* uart_path )
: _m_gps_(uart_path, ::gps_pkg::Cgps::Tbr::E_BR_115200) {
    clear();

    try {
        bool time_on = false;
        double gps_time = 0.0;

        _m_holding_time_ = holding_time_on;
        _m_myself_ = myself;
        _mf_send_ = func;
        if( _m_myself_.get() == NULL ) {
            throw std::invalid_argument("myself argument is empty value.");
        }
        if( _mf_send_ == NULL ) {
            _m_myself_.reset();
            throw std::invalid_argument("sender function is empty.");
        }

        // Init state
        _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_SYNC, 0);
        _m_myself_->set_state(::common::E_STATE::E_STATE_THR_KEEPALIVE, 0);
        set_time_state(time_on, false, !time_on);

        // if possible, then set System-Time according to GPS-Time.
        gps_time = get_time_src();
        if( gps_time > 0.0 ) {
            set_system_time( gps_time );
            time_on = true;
            set_time_state(time_on, true, !time_on);
        }
        
        create_threads();
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

CTimeSync::~CTimeSync( void ) {
    destroy_threads();
    clear();
}

/* Only for Client */
bool CTimeSync::regist_keepalive( std::string peer_app, std::string peer_pvd, std::shared_ptr<ICommunicator>& comm ) {
    bool result = true;
    try {
        std::shared_ptr<CServerInfo> server;
        std::string full_path = alias_full_path(peer_app, peer_pvd);

        if( comm.get() == NULL ) {
            throw std::invalid_argument("communicator instance is NULL");
        }

        {
            std::lock_guard<std::mutex>  guard(_mtx_servers_);
            if( _mm_servers_.find(full_path) != _mm_servers_.end() ) {
                LOGW("Already the peer(%s) is registered to Wanted-Peer list for KeepAlive-proc.", full_path.data());
                return result;
            }

            // create CServerInfo
            server = std::make_shared<CServerInfo>(peer_app, peer_pvd, comm);
            if( server.get() == NULL ) {
                throw std::runtime_error("Can not memory allocation to CServerInfo.");
            }

            // try to connect with peer & regist peer to wanted-list.
            server->connect_try();
            auto ret = _mm_servers_.insert(std::pair<std::string,std::shared_ptr<CServerInfo>>(full_path, server));
            if( ret.second == false ) {
                std::string err = "Can not insert new peer. Because already exist key.(" + full_path + ")";
                throw std::logic_error(err);
            }
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        result = false;
    }

    return result;
}

/* Only for Client */
bool CTimeSync::unregist_keepalive( const std::string& peer_app, const std::string& peer_pvd ) {
    try{
        std::shared_ptr<CServerInfo> server;
        std::string full_path = alias_full_path(peer_app, peer_pvd);
        
        {
            std::lock_guard<std::mutex>  guard(_mtx_servers_);
            // remove peer from _mm_servers_
            auto itr = _mm_servers_.find( full_path );
            if( itr == _mm_servers_.end() ) {
                LOGW("peer(%s) is not exist.", full_path.data());
                return false;
            }

            server = itr->second;
            _mm_servers_.erase(itr);
        }

        // disconnect peer
        server->disconnect();
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return true;
}

void CTimeSync::regist_failsafe(TFfailSafe func) {
    if( func == NULL ) {
        throw std::logic_error("\"func\" is NULL.");
    }
    _mf_failSafe_ = func;
}

void CTimeSync::unregist_failsafe(void) {
    _mf_failSafe_ = NULL;
}

void CTimeSync::regist_cb_service_state( TFsvcState func ) {
    try {
        bool time_on = false;
        if( func == NULL ) {
            throw std::logic_error("\"func\" is NULL.");
        }
        _mf_svcState_ = func;

        // Notify first service-state indicator.
        time_on = _m_myself_->get_state(::common::E_STATE::E_STATE_TIME_ON) != 0 ? true : false;
        LOGI("Notify Service-state info. (time_on=%d)", time_on);
        _mf_svcState_( time_on );
    }
    catch (const std::exception& e ) {
        LOGERR("%s", e.what());
        throw ;
    }
}

void CTimeSync::append_peer(std::string app, std::string pvd, double sent_time, double rcv_time) {
    try {
        bool need_update_time = false;
        std::string full_path = alias_full_path(app, pvd);
        CpeerDesc peer(app, pvd, sent_time, rcv_time);
        
        {
            std::lock_guard<std::mutex>  guard(_mtx_peers_);
            if( _mm_peers_.find(full_path) != _mm_peers_.end() ) {
                LOGW( "%s is already exist as peer.", full_path.data() );
                return ;
            }

            // Insert new peer to _mm_peers_ mapper.
            auto ret = _mm_peers_.insert(std::pair<std::string,CpeerDesc>(full_path, peer));
            if( ret.second == false ) {
                std::string err = "Can not insert new peer. Because already exist key.(" + full_path + ")";
                throw std::logic_error(err);
            }

            if( _mm_peers_.size() == 1 ) {
                need_update_time = true;
            }
        }

        if( need_update_time == true ) {
            update_time();
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CTimeSync::remove_peer(std::string app, std::string pvd) {
    try {
        std::lock_guard<std::mutex>  guard(_mtx_peers_);

        auto itr = _mm_peers_.find( alias_full_path(app, pvd) );
        if( itr != _mm_peers_.end() ) {
            auto& target = (itr->second).alias;

            LOGI("remove target(%u/%u) from mapper.", app.data(), pvd.data());
            if( target->app_path != app || target->pvd_id != pvd ) {
                std::string err = "peer-mapper has invalid-mapping about target(" + target->app_path + "/" + target->pvd_id + ")";
                throw std::logic_error(err);
            }
            _mm_peers_.erase(itr);
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CTimeSync::update_keepalive(std::shared_ptr<CuCMD> cmd) {
    auto lamda_send_keepalive = [&](std::string& peer_full_path) -> void {
        std::lock_guard<std::mutex>  guard(_mtx_peers_);
        auto itr = _mm_peers_.find( peer_full_path );
        if( itr != _mm_peers_.end() ) {
            auto& target = itr->second.alias;
            LOGD("target-id=%s/%s", target->app_path.data(), target->pvd_id.data());

            if( _mf_send_(*target, _m_myself_->get_machine_name(), ::common::E_STATE::E_NO_STATE) == 0 ) {
                LOGERR("Sending Keep-Alive message is failed. target=%s/%s", target->app_path.data(), target->pvd_id.data() );
            }
        }
    };

    try {
        if( cmd.get() == NULL ) {
            throw std::invalid_argument("cmd is empty.");
        }

        if( cmd->get_flag(::common::E_FLAG::E_FLAG_KEEPALIVE) != ::common::E_FLAG::E_FLAG_KEEPALIVE ) {
            throw std::invalid_argument("cmd is not KEEPALIVE. It's invalid-command.");
        }

        bool need_update_time = false;
        std::string app = cmd->get_from().app_path;
        std::string pvd = cmd->get_from().pvd_id;
        std::string full_path = alias_full_path(app,pvd);
        double rcv_time = cmd->get_rcv_time();
        double sent_time = cmd->get_send_time();
        std::string payload  = cmd->get_payload();
        uint16_t state = cmd->get_state();

        {   // update property about target-peer.
            bool value = false;
            std::lock_guard<std::mutex>  guard(_mtx_peers_);

            auto itr = _mm_peers_.find( full_path );
            if( itr == _mm_peers_.end() ) {
                CpeerDesc peer(app, pvd, sent_time, rcv_time);
                auto ret = _mm_peers_.insert(std::pair<std::string,CpeerDesc>(full_path, peer));
                if( ret.second == false ) {
                    std::string err = "Already exist key.(" + full_path + ")";
                    throw std::logic_error(err);
                }
                itr = ret.first;

                if( _mm_peers_.size() == 1 ) {
                    need_update_time = true;
                }
            }

            auto& target = itr->second;

            target.rcv_time = rcv_time;
            target.sent_time = sent_time;
            target.alias->set_machine_name( payload );
            
            value = (state & ::common::E_STATE::E_STATE_TIME_ON) != 0 ? true : false;
            target.alias->set_state(::common::E_STATE::E_STATE_TIME_ON, value);
            value = (state & ::common::E_STATE::E_STATE_TIME_SRC) != 0 ? true : false;
            target.alias->set_state(::common::E_STATE::E_STATE_TIME_SRC, value);
        }

        // check state & trig update_time proc.
        if( need_update_time == true || 
            (state & ::common::E_STATE::E_STATE_TIME_SYNC) != 0 ) {
            update_time();
            lamda_send_keepalive(full_path);
        }

        // check peer-keepalive about time_sync & react about it if need.
        react_4_notified_time_sync(app, pvd);
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
}


/**********************************
 * Definition of Private Function.
 */
void CTimeSync::clear(void) {
    _m_myself_.reset();
    _mf_send_ = NULL;
    _mf_failSafe_ = NULL;
    _mf_svcState_ = NULL;

    // Thread routine variables
    _m_is_continue_ = false;       // Thread continue-flag.
    _m_watchdog_ = 0.0;
    _m_holding_time_ = 0.0;

    _mm_servers_.clear();
    _mm_peers_.clear();
    _mm_unsynced_peers_.clear();
}

bool CTimeSync::check_holding_service(bool time_src, bool time_on, bool& cur_time_on) {
    /****************************************************/
    /* Check wether maintain Time-ON or Out-of-Service. */
    //
    // If time-source == false && myself.get_state()!=SRC && watchdog-timer is enabled, 
    //    then compare watchdog-tim & now-time      return watchdog-overflow ? time-on : true.
    // If time-source == false && myself.get_state()!=SRC && watchdog-timer is disabled, 
    //    then                                      return time-on.
    // If time-source == false && myself.get_state()==SRC, 
    //    then watchdog-timer is enabled,           return true.
    // If time-source == true, 
    //    then reset of watchdog-timer is trigged   return time-on.

    try {
        cur_time_on = false;
        if( _m_myself_->get_state(::common::E_STATE::E_STATE_TIME_ON) != 0 ) {
            cur_time_on = true;
        }

        if( time_src == true ) {
            // reset WatchDog-Timer
            _m_watchdog_ = 0.0;
            return time_on;
        }

        // If cur_source == false && pre_state == TIME_SRC, then enable WatchDog-Timer.
        if( _m_myself_->get_state(::common::E_STATE::E_STATE_TIME_SRC) != 0 ) {
            // enable WatchDog-Timer
            _m_watchdog_ = time_pkg::CTime::get<double>() + _m_holding_time_;
            return true;
        }

        if( _m_watchdog_ == 0.0 ) {
            return time_on;
        }

        /* if WatchDog-Timer is overflowed */
        return (_m_watchdog_ <= time_pkg::CTime::get<double>())? time_on : true;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CTimeSync::set_time_state(bool time_on, bool time_src, bool cur_time_on) {
    // set state TIME_SRC, TIME_ON, OUT_OF_SERVICE
    _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_ON, time_on);
    _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_SRC, time_src);
    _m_myself_->set_state(::common::E_STATE::E_STATE_OUT_OF_SERVICE, !time_on);

    if( (cur_time_on != time_on) && _mf_svcState_ != NULL ) {
        LOGI("Notify Service-state info. (time_on=%d)", time_on);
        _mf_svcState_( time_on );
    }
}

void CTimeSync::update_peer(void) {
    try {
        double now = time_pkg::CTime::get<double>();
        std::lock_guard<std::mutex>  guard(_mtx_peers_);

        for( auto itr=_mm_peers_.begin(); itr != _mm_peers_.end(); ) {
            auto& target = itr->second;

            if( now - target.rcv_time > TIME_TREATE_AS_DISCONNACT ) {
                LOGI("Treat target-id(=%s/%s) as disconnected peer", target.alias->app_path.data(), target.alias->pvd_id.data());
                itr = _mm_peers_.erase( itr );
                continue ;
            }
            itr++;
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
}

bool CTimeSync::update_time(void) {         // locking API
    bool result = false;
    try {
        // calculate average-time with peer now-sent-time , self gps_time.
        bool cur_time_on = false;
        bool time_src = false;
        bool time_on = false;
        double now = 0.0;
        double avg_time = calculate_avg_time_on( now, time_on, time_src );

        // update system-time if need it. (gap >= 0.2 sec)
        double gap = avg_time - now;
        if( set_system_time(avg_time, 0.2, now) == true ) {
            result = true;
            
            // update 'rcv_time' within _mm_peers_.
            std::lock_guard<std::mutex>  guard(_mtx_peers_);
            for( auto itr=_mm_peers_.begin(); itr != _mm_peers_.end(); itr++ ) {
                itr->second.rcv_time += gap;
            }
        }

        // check maintain TIME_ON or Out-of-service.
        time_on = check_holding_service( time_src, time_on, cur_time_on );
        set_time_state(time_on, time_src, cur_time_on);

        if( result == true ) {
            notify_update();
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

void CTimeSync::notify_update(void) {
    auto lamda_send_keepalive_time_sync = [&](std::shared_ptr<alias::CAlias>& peer) -> void {
        if(_m_myself_->get_state(::common::E_STATE::E_STATE_TIME_ON) == 0) {
            LOGW("It's not on TIME_ON state. So, it's not qualified to send KEEPALIVE with TIME_SYNC.");
            return ;
        }

        // send KEEPALIVE with TIME_SYNC flag.
        if( _mf_send_(*peer, _m_myself_->get_machine_name(), ::common::E_STATE::E_STATE_TIME_SYNC) == 0 ) {
            LOGERR("Sending Keep-Alive message is failed. (with TIME_SYNC) peer=%s/%s", peer->app_path.data(), peer->pvd_id.data() );
        }

        // register target to wait that receiving KEEPALIVE from target.
        _mm_unsynced_peers_[ alias_full_path(peer->app_path, peer->pvd_id) ] = true;
    };

    try {
        double gap = 0.0;
        // Send keepalive message with TIME_SYNC state for unsynced peers.
        std::lock_guard<std::mutex>  guard(_mtx_peers_);

        for( auto itr=_mm_peers_.begin(); itr != _mm_peers_.end(); itr++ ) {
            auto& target = itr->second;
            LOGD("target-id=%s/%s", target.alias->app_path.data(), target.alias->pvd_id.data());

            gap = fabs(target.rcv_time - target.sent_time);
            if( gap >= TIME_INTERVAL_THRESOLDER ) {
                lamda_send_keepalive_time_sync(target.alias);
            }
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CTimeSync::react_4_notified_time_sync(std::string app, std::string pvd) {
    try {
        std::lock_guard<std::mutex>  guard(_mtx_peers_);

        auto itr = _mm_unsynced_peers_.find( alias_full_path(app, pvd) );
        if( itr != _mm_unsynced_peers_.end() ) {
            _mm_unsynced_peers_.erase(itr);

            // Call Listener of CMD-scheduler to check Now-DB & process Trouble-shooting for unsynced peers.
            if( _mf_failSafe_ != NULL ) {
                _mf_failSafe_(app, pvd);
            }
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

int CTimeSync::run_keepalive(void) {    // Keep-Alive react Thread-routin.
    int64_t cur_time = 0;
    auto lamda_check_update_time = [&](void) {
        cur_time += TIME_SEND_PERIOD_KEEPALIVE;

        if( cur_time >= TIME_UPDATE_PERIOD ) {
            update_time();
            cur_time = 0;
        }
    };
    auto lamda_check_wanted_connection = [&](void) {
        std::vector<std::shared_ptr<CServerInfo>> wanted_list;
        {
            std::lock_guard<std::mutex>  guard(_mtx_servers_);
            for( auto itr=_mm_servers_.begin(); itr!= _mm_servers_.end(); itr++ ) {
                {
                    std::lock_guard<std::mutex>  guard(_mtx_peers_);
                    if( _mm_peers_.find(itr->first) != _mm_peers_.end() ) {
                        continue;
                    }
                }

                // insert server that is not yet connected with me.
                wanted_list.push_back(itr->second);
            }
        }

        // Try to connect with wanted servers.
        for( auto itr=wanted_list.begin(); itr!=wanted_list.end(); itr++ ) {
            LOGI("Try to reconnect to peer.(%s)", (*itr)->name().data());
            (*itr)->connect_try();
        }
    };

    while(_m_is_continue_.load()) {
        try {
            update_peer();
            lamda_check_update_time();
            lamda_check_wanted_connection();

            {
                std::lock_guard<std::mutex>  guard(_mtx_peers_);
                for( auto itr=_mm_peers_.begin(); itr != _mm_peers_.end(); itr++ ) {
                    auto& target = (itr->second).alias;
                    LOGD("target-id=%s/%s", target->app_path.data(), target->pvd_id.data());

                    if( _mf_send_(*target, _m_myself_->get_machine_name(), ::common::E_STATE::E_NO_STATE) == 0 ) {
                        LOGERR("Sending Keep-Alive message is failed. target=%s/%s", target->app_path.data(), target->pvd_id.data() );
                    }
                }
            }

            // wait 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(TIME_SEND_PERIOD_KEEPALIVE));
        }
        catch (const std::exception &e) {
            LOGERR("%s", e.what());
        }
    }

    LOGI("Exit Keep-Alive thread.");
    return 0;
}

void CTimeSync::create_threads(void) {
    try {
        if( _m_is_continue_.exchange(true) == false ) {
            LOGI("Create Keep-Alive thread.");
            _mt_keepaliver_ = std::thread(&CTimeSync::run_keepalive, this);

            if ( _mt_keepaliver_.joinable() == false ) {
                _m_is_continue_ = false;
                throw std::runtime_error("run_keepalive thread creating is failed.");
            }
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CTimeSync::destroy_threads(void) {
    if( _m_is_continue_.exchange(false) == true ) {
        if( _mt_keepaliver_.joinable() == true ) {
            LOGI("Destroy Keep-Alive thread.");     // Destroy of KEEP-ALIVE reacting thread.
            _mt_keepaliver_.join();
        }
    }
}

std::string CTimeSync::alias_full_path(const std::string& app, const std::string& pvd) {
    return app + "/" + pvd;
}

double CTimeSync::get_time_src(void) {
    if( _m_gps_.is_active() ) {
        return _m_gps_.get_time();
    }
    return 0.0;
}

bool CTimeSync::set_system_time( double time, double gap_threshold, double now ) {
    bool result = false;

    if ( time <= 0.0 ) {
        LOGW("\"time(%f)\" is invalid value.", time);
        return result;
    }

    if( now == 0.0 ) {
        now = ::time_pkg::CTime::get<double>();
    }

    if( fabs(time - now) < gap_threshold ) {
        LOGW("Gap about the time is under the threshold(%f)", gap_threshold);
        return result;
    }

    // Set time as system-time.
    result = ::time_pkg::CTime::set( time );
    if( result == true ) {
        _m_gps_.reset();
    }
    return result;
}

double CTimeSync::calculate_avg_time_on( double& now, bool& time_on, bool& time_src ) {
    typedef struct _s_double_sum_ {
        double sum;
        uint32_t cnt;
    
        _s_double_sum_(void) {
            sum = 0.0;
            cnt = 0;
        }
    } SSumDouble;

    double avg_time_on = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = 0.0;
    double sum = 0.0;
    auto lamda_calc_min_max_sum = [&](std::map<std::string/*machine*/,SSumDouble>& now_time_on) -> uint32_t {
        for( auto itr=now_time_on.begin(); itr != now_time_on.end(); itr++ ) {
            auto& target = itr->second;

            double value = target.sum / (double)(target.cnt);
            min = value < min ? value : min;    // update min value
            max = value > max ? value : max;    // update max value
            sum += value;
        }
        return now_time_on.size();
    };

    try {
        std::map<std::string/*machine*/,SSumDouble> now_time_on;
        double tsrc_value = get_time_src();
        now = ::time_pkg::CTime::get<double>();
        time_src = false;

        /** Sum of gps-time per MACHINE_NAME. */
        if( tsrc_value != 0.0 ) {
            time_src = true;
            now_time_on[_m_myself_->get_machine_name()].sum += tsrc_value;
            now_time_on[_m_myself_->get_machine_name()].cnt += 1;
        }

        {
            std::lock_guard<std::mutex>  guard(_mtx_peers_);
            for( auto itr=_mm_peers_.begin(); itr != _mm_peers_.end(); itr++ ) {
                auto& target = itr->second;

                // update peer-sent-time of TIME_ON peers.
                if( target.alias->get_state(::common::E_STATE::E_STATE_TIME_ON) != 0 ) {
                    double value = target.sent_time + (now - target.rcv_time);
                    now_time_on[target.alias->get_machine_name()].sum += value;
                    now_time_on[target.alias->get_machine_name()].cnt += 1;
                }
            }
        }

        /** Get average gps-time */
        auto count = lamda_calc_min_max_sum( now_time_on );
        if( count >= 3 ) {
            avg_time_on = (sum - min - max) / (double)(count-2);
            time_on = true;
        }
        else if( count > 0 ) {
            avg_time_on = sum / (double)(count);
            time_on = true;
        }
        else {
            avg_time_on = now;
            time_on = false;
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    
    return avg_time_on;
}


}   // cmd
