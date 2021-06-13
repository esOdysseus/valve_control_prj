#ifndef _C_TRANCEIVER_COMMAND_H_
#define _C_TRANCEIVER_COMMAND_H_

#include <list>
#include <string>
#include <thread>
#include <memory>
#include <condition_variable>

#include <IAppInf.h>
#include <CVctrlCMD.h>
#include <CDebugCMD.h>
#include <CProcValve.h>
#include <CProcService.h>
#include <CProcState.h>

namespace cmd_pkg {

class CTranceiverCMD {
public:
    using CommHandler = std::shared_ptr<ICommunicator>;
    using CMDDebug = cmd_pkg::CDebugCMD;
    using CMDType = cmd_pkg::CVctrlCMD;
    using CMDlistType = std::list<std::shared_ptr<CMDDebug>>;
    static constexpr const char* PBigEdian = "CPBigEndian";
    static constexpr const char* PLittleEndian = "CPLittleEndian";

public:
    static std::shared_ptr<CTranceiverCMD> get_instance(CommHandler *handler=NULL);

    ~CTranceiverCMD(void);

    void exit(void);

    void cb_initialization(enum_c::ProviderType provider_type, bool flag_init);

    void cb_connected(std::string client_id, bool flag_connect);

    // Control valves according to command of Center-Brain.
    void cb_receive_msg_handle(std::string client_id, std::shared_ptr<payload::CPayload> payload);

    void cb_abnormally_quit(const std::exception &e);

    static bool insert_reloaded_cmd(std::shared_ptr<CMDDebug> &cmd);

private:
    CTranceiverCMD(void) = delete;

    CTranceiverCMD(CommHandler handler);

    void clear(void);

    // with regard to Thread
    bool create_threads(void);

    void destroy_threads(void);

    bool update_trig(std::shared_ptr<CMDType> &cmd);

    bool execute_cmd(std::shared_ptr<CMDDebug> &cmd);

    int run_cmd_execute(void); // Thread: Execute command routin.

    // with regard to CMD
    bool insert_new_cmd(std::shared_ptr<CMDDebug> cmd);

    bool send(std::string who, const void *data, ssize_t data_size);

private:
    // for terminate CTranceiverCMD class.
    bool _is_exit_;
    
    // for communicator
    CommHandler _h_communicator_; // Communicator-handler.

    // for thread of command-execution
    bool _is_continue_;       // Thread continue-flag.

    std::thread _runner_exe_cmd_; // Periodically, Thread that is charge of deciding & executing for received CMD.

    CMDlistType _cmd_list_;     // cmd encode/decode for valve-controling.

    std::mutex _mtx_cmd_list_;

    std::condition_variable _wake_exe_cmd_;

    // for update data
    proc_pkg::CProcValve _proc_valve_;

    proc_pkg::CProcService _proc_svc_;

    proc_pkg::CProcState _proc_state_;

};

}   // namespace cmd_pkg


#endif // _C_TRANCEIVER_COMMAND_H_