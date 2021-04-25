#ifndef _INTERFACE_PROCESS_H_
#define _INTERFACE_PROCESS_H_

#include <map>
#include <list>
#include <memory>
#include <thread>
#include <condition_variable>

#include <CVctrlCMD.h>
#include <CDebugCMD.h>
#include <IProcShare.h>

namespace monitor_pkg {
    struct Cmsg_packet;
}

namespace proc_pkg {


class IProc {
public:
    using CMDType = cmd_pkg::CVctrlCMD;
    using CMDDebug = cmd_pkg::CDebugCMD;
    using PacketType = monitor_pkg::Cmsg_packet;
    using ShareProcsType = IProcShare;
    using TaskType = enum E_PROC_TASK {
        E_PROC_TASK_DEFAULT = 0,
        E_PROC_TASK_TIME_UPDATE = 1,
        E_PROC_TASK_SAVE_FILE = 2
    };

protected:
    template <typename T> 
    class CCMD_ST {
    public:
        TaskType task_type;
        std::shared_ptr<T> cmd;
    
        CCMD_ST(std::shared_ptr<T> cmd, TaskType type=TaskType::E_PROC_TASK_DEFAULT)
        : task_type(type), cmd(cmd){};
    }; 

    using UCMD_STType = CCMD_ST<CMDType>;
    using UCMDlistType = std::list<std::shared_ptr<UCMD_STType>>;
    using DCMD_STType = CCMD_ST<CMDDebug>;
    using DCMDlistType = std::list<std::shared_ptr<DCMD_STType>>;

public:
    IProc(std::string proc_name);

    ~IProc(void);

    bool init(std::string &myself_name, std::shared_ptr<ShareProcsType> instance);

    std::string get_proc_name(void);

    void destroy_threads(void);

    template <typename T>
    bool insert_cmd(std::shared_ptr<T> &cmd, TaskType type=TaskType::E_PROC_TASK_DEFAULT);

    // Making packet to be send
    virtual std::shared_ptr<PacketType> make_packet_for_dbgcmd(std::shared_ptr<CMDDebug> &cmd) = 0;

    // Register result of sending packet made by 'make_packet_for_dbgcmd'.
    virtual void register_sent_msg(uint32_t msg_id, bool result) = 0;

protected:
    std::string get_app_name(void);

    IProc* find_proc(std::string name);

    // Thread core-routine to update data.
    virtual bool data_update(TaskType type, std::shared_ptr<CMDType> cmd) = 0;

    virtual bool create_custom_threads(void);

    virtual void destroy_custom_threads(void);

private:
    IProc(void) = delete;

    bool create_threads(void);
    
    void clear(void);

    int run_update_data(void); // Thread: Data Update-routin.

protected:
    bool _is_continue_;       // Thread continue-flag.

    bool _is_custom_thread_;

    // for DBGK-CMD.    
    DCMDlistType _dbgk_cmd_list_;

    std::mutex _mtx_dbgk_cmd_list_;

private:
    std::string _proc_name_;

    bool _is_init_;

    std::string _app_name_;

    // for mesh-struct IProc-instances
    std::shared_ptr<ShareProcsType> _procs_map_;

    // for thread of command-execution
    std::thread _runner_update_; // Eventually, Thread that is charge of deciding & executing for received CMD.

    // for UCMD-CMD.
    UCMDlistType _cmd_list_;     // cmd encode/decode for valve-controling.

    std::mutex _mtx_cmd_list_;

    std::condition_variable _wake_runner_;

};




}

#endif // _INTERFACE_PROCESS_H_