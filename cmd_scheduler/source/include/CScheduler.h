#ifndef _CLASS_SCHEDULER_SERVICE_H_
#define _CLASS_SCHEDULER_SERVICE_H_

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>

#include <MCommunicator.h>
#include <ICommand.h>
#include <CDBhandler.h>

namespace service {


class CScheduler {
public:
    static const std::string APP_PATH;
    static const std::string PVD_COMMANDER;
    static const std::string PVD_DEBUGGER;

private:
    using Tdb = db::CDBhandler;

public:
    static std::shared_ptr<CScheduler> get_instance( void );

    bool init( std::string file_path_alias, std::string file_path_proto );

    void start( void );

    void exit( void );

    ~CScheduler( void );

private:
    CScheduler(const CScheduler&) = delete;             // copy constructor
    CScheduler& operator=(const CScheduler&) = delete;  // copy operator
    CScheduler(CScheduler&&) = delete;                  // move constructor
    CScheduler& operator=(CScheduler&&) = delete;       // move operator

    CScheduler( void );

    void clear( void );

    void receive_command( std::shared_ptr<cmd::ICommand>& cmd );

    void send_command( const std::string& peer_app, const std::string& peer_pvd, 
                       const std::string& json_data );

    void push_cmd( std::shared_ptr<cmd::ICommand>& cmd );

    std::shared_ptr<cmd::ICommand> pop_cmd( void );     // Blocking function.

    /** Thread releated Functions. */
    void create_threads(void);

    void destroy_threads(void);

    int handle_rx_cmd(void);

    int handle_tx_cmd(void);

private:
    std::shared_ptr<comm::MCommunicator>  _m_comm_mng_;

    Tdb _m_db_;

    /* Thread Routin variables */
    std::atomic<bool> _m_is_continue_;

    std::thread _mt_rcmd_handler_;       // Store of Received CMD. (Rx)

    std::thread _mt_scmd_handler_;       // Trig of Send CMD. (Tx)

    /** Blocking Queue for received CMDs */
    std::mutex _mtx_queue_lock_;
    std::condition_variable _m_queue_cv_;
    std::queue<std::shared_ptr<cmd::ICommand>> _mv_cmds_;

};


}   // namespace service


#endif // _CLASS_SCHEDULER_SERVICE_H_