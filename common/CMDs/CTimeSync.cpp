#include <math.h>
#include <limits>

#include <logger.h>
#include <CMDs/CTimeSync.h>
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


constexpr const double CTimeSync::TIME_TREATE_AS_DISCONNACT;
constexpr const int64_t CTimeSync::TIME_SEND_PERIOD_KEEPALIVE;
constexpr const int64_t CTimeSync::TIME_UPDATE_PERIOD;
constexpr const double CTimeSync::TIME_INTERVAL_THRESOLDER;


/**********************************
 * Definition of Public Function.
 */
CTimeSync::CTimeSync( std::shared_ptr<::alias::CAlias>& myself, TFsend func )
: _m_gps_(GPS_UART_PATH) {
    clear();

    try {
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
        _m_myself_->set_state(::common::E_STATE::E_STATE_OUT_OF_SERVICE, 1);
        _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_ON, 0);
        _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_SRC, 0);

        // if possible, then set System-Time according to GPS-Time.
        if( set_system_time( get_time_src() ) == true ) {
            _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_ON, 1);
            _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_SRC, 1);
            _m_myself_->set_state(::common::E_STATE::E_STATE_OUT_OF_SERVICE, 0);
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

void CTimeSync::regist_failsafe(TFfailSafe func) {
    _mf_failSafe_ = func;
}

void CTimeSync::unregist_failsafe(void) {
    _mf_failSafe_ = NULL;
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

    // Thread routine variables
    _m_is_continue_ = false;       // Thread continue-flag.
    _mm_peers_.clear();
    _mm_unsynced_peers_.clear();
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
        bool time_src = false;
        bool time_on = false;
        double now = 0.0;
        double avg_time = calculate_avg_time_on( now, time_on, time_src );

        // update system-time if need it. (gap >= 0.2 sec)
        double gap = avg_time - now;
        if( fabs(gap) >= 0.2 ) {
            if( set_system_time(avg_time) == true ) {
                result = true;
                
                // update 'rcv_time' within _mm_peers_.
                std::lock_guard<std::mutex>  guard(_mtx_peers_);
                for( auto itr=_mm_peers_.begin(); itr != _mm_peers_.end(); itr++ ) {
                    itr->second.rcv_time += gap;
                }
            }
        }

        // set state TIME_SRC, TIME_ON, OUT_OF_SERVICE
        _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_ON, time_on);
        _m_myself_->set_state(::common::E_STATE::E_STATE_TIME_SRC, time_src);
        _m_myself_->set_state(::common::E_STATE::E_STATE_OUT_OF_SERVICE, !time_on);
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

    while(_m_is_continue_.load()) {
        try {
            update_peer();
            lamda_check_update_time();

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

std::string CTimeSync::alias_full_path(std::string& app, std::string& pvd) {
    return app + "/" + pvd;
}

double CTimeSync::get_time_src(void) {
    if( _m_gps_.is_active() ) {
        return _m_gps_.get_time();
    }
    return 0.0;
}

bool CTimeSync::set_system_time( double time ) {
    if ( time <= 0.0 ) {
        LOGW("\"time(%f)\" is invalid value.", time);
        return false;
    }

#ifdef TEST_MODE_ENABLE
    return true;
#else
    return ::time_pkg::CTime::set( time );
#endif
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
