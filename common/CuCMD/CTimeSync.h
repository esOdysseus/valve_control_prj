#ifndef _H_CLASS_TIME_SYNCHRONIZER_H_
#define _H_CLASS_TIME_SYNCHRONIZER_H_

#include <map>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <functional>

#include <Common.h>
#include <CuCMD/CuCMD.h>
#include <time_kes.h>
#include <gps.h>
#include <ICommunicator.h>

/*******************************
 * Definition of Class.
 */
namespace cmd {


class CTimeSync {
public:
    using TFsend = std::function<uint32_t( const alias::CAlias& /*peer*/, const std::string& /*data*/, common::StateType /*state*/ )>;
    using TFfailSafe = std::function<void(std::string&/*app*/, std::string&/*pvd*/)>;

private:
    class CpeerDesc {
    public:
        std::shared_ptr<::alias::CAlias> alias;
        double rcv_time;    // received-time by self
        double sent_time;   // sent-time by peer

        CpeerDesc( std::string& peer_app, std::string& peer_pvd, double _sent_time_, double _rcv_time_ ) {
            alias = std::make_shared<alias::CAlias>(peer_app, peer_pvd);
            rcv_time = _rcv_time_;
            sent_time = _sent_time_;

            if( rcv_time <= 0.0 ) {
                rcv_time = ::time_pkg::CTime::get<double>();
            }
        }

        ~CpeerDesc(void) {
            alias.reset();
            rcv_time = 0.0;
            sent_time = 0.0;
        }

    private:
        CpeerDesc(void) = delete;

    };

public:
    CTimeSync( std::shared_ptr<::alias::CAlias>& myself, TFsend func );

    ~CTimeSync( void );

    /* Only for Client */
    bool regist_keepalive( std::string peer_app, std::string peer_pvd, std::shared_ptr<ICommunicator>& comm );

    /* Only for Client */
    bool unregist_keepalive( const std::string& peer_app, const std::string& peer_pvd );

    void regist_failsafe(TFfailSafe func);

    void unregist_failsafe(void);

    /* When happen connected-call-back, this function will be called. */
    void append_peer(std::string app, std::string pvd, double sent_time=0.0, double rcv_time=0.0);

    /* When happen disconnected-call-back, this function will be called. */
    void remove_peer(std::string app, std::string pvd);

    /* When received keepalive-msg, this function will be called. */
    void update_keepalive(std::shared_ptr<CuCMD> cmd);

private:
    CTimeSync( void ) = delete;

    void clear(void);
    
    void update_peer(void);

    bool update_time(void);

    void notify_update(void);

    void react_4_notified_time_sync(std::string app, std::string pvd);

    int run_keepalive(void);    // Keep-Alive react routin.

    void create_threads(void);

    void destroy_threads(void);

    static std::string alias_full_path(const std::string& app, const std::string& pvd);

    double get_time_src(void);

    bool set_system_time( double time );

    double calculate_avg_time_on( double& now, bool& time_on, bool& time_src );

private:
    std::shared_ptr<::alias::CAlias> _m_myself_;    // MCommunicator state

    TFsend _mf_send_;

    TFfailSafe _mf_failSafe_;

    std::map<std::string/*peer-full-path*/, bool> _mm_unsynced_peers_;     // Connected Peer-list. (for keep-alive proc)

    ::gps_pkg::Cgps _m_gps_;

    // Thread routine variables
    std::atomic<bool> _m_is_continue_;       // Thread continue-flag.

    std::thread _mt_keepaliver_; // Periodically, Thread that is charge of reacting Keep-Alive message.

    class CServerInfo;
    std::map<std::string/*peer-full-path*/, std::shared_ptr<CServerInfo>> _mm_servers_;   // Wanted Peer-list. (for keep-alive proc)
    std::mutex _mtx_servers_;

    std::map<std::string/*peer-full-path*/, CpeerDesc> _mm_peers_;     // Connected Peer-list. (for keep-alive proc)
    std::mutex _mtx_peers_;

    static constexpr const double TIME_TREATE_AS_DISCONNACT = 10.0;
    static constexpr const int64_t TIME_SEND_PERIOD_KEEPALIVE = 5;
    static constexpr const int64_t TIME_UPDATE_PERIOD = 60;
    static constexpr const double TIME_INTERVAL_THRESOLDER = 0.2;
    static constexpr const char* GPS_UART_PATH = "/dev/ttyS1";

};


}   // namespace cmd


#endif // _H_CLASS_TIME_SYNCHRONIZER_H_