#ifndef _C_PROCESS_STATE_H_
#define _C_PROCESS_STATE_H_

#include <IProc.h>

namespace proc_pkg {


class CProcState: public IProc {
public:
    static constexpr const char *SELF_NAME = "CProcState";

public:
    CProcState(void);

    ~CProcState(void);

    // Making packet to be send
    std::shared_ptr<PacketType> make_packet_for_dbgcmd(std::shared_ptr<CMDDebug> &cmd) override;

    // Register result of sending packet made by 'make_packet_for_dbgcmd'.
    void register_sent_msg(uint32_t msg_id, bool result) override;

    bool append_time_for_updating(double time_in_packet, double time_on_receiv);

protected:
    // Thread core-routine to update data.
    bool data_update(TaskType type, std::shared_ptr<CMDType> cmd) override;

};


}

#endif // _C_PROCESS_STATE_H_