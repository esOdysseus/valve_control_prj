#ifndef _C_PROCESS_SERVICE_H_
#define _C_PROCESS_SERVICE_H_

#include <IProc.h>
#include <CSvcMonitor.h>

namespace proc_pkg {


class CProcService: public IProc {
public:
    static constexpr const char *SELF_NAME = "CProcService";

private:
    using MNT = monitor_pkg::CSvcMonitor;
    using MNT_FLAG = MNT::E_MNT_FLAG;
    using MNT_ERROR = MNT::E_MNT_ERROR;

public:
    CProcService(void);

    ~CProcService(void);
    
    // Making packet to be send
    std::shared_ptr<PacketType> make_packet_for_dbgcmd(std::shared_ptr<CMDDebug> &cmd) override;

    // Register result of sending packet made by 'make_packet_for_dbgcmd'.
    void register_sent_msg(uint32_t msg_id, bool result) override;

    bool is_available_service(std::string app_name);

protected:
    // Thread core-routine to update data.
    bool data_update(TaskType type, std::shared_ptr<CMDType> cmd) override;

    bool create_custom_threads(void) override;

    void destroy_custom_threads(void) override;

private:
    int run_period(void);        // Thread: Period-running routin.

    bool validation_check(std::shared_ptr<CMDType> cmd);

private:
    MNT _monitor_;

    std::thread _runner_period_;

    static constexpr double   MAX_DELAY_FOR_DISCONNECT = 15.0;

};


}

#endif // _C_PROCESS_SERVICE_H_