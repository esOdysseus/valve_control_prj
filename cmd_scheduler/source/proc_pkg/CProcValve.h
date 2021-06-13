#ifndef _C_PROCESS_VALVE_H_
#define _C_PROCESS_VALVE_H_

#include <list>

#include <IProc.h>
#include <CPacketMonitor.h>
#include <CDBGKargVctrl.h>

namespace proc_pkg {

class CProcValve: public IProc {
public:
    static constexpr const char *SELF_NAME = "CProcValve";
    static constexpr const unsigned int CMD_RELOADING_PERIOD = 60;   // unit : second
    static constexpr const char *DBGK_PERIOD_CMDS_FILE = "period_cmds.txt";
    static constexpr const char *DBGK_EVENT_CMDS_FILE = "event_cmds.txt";

public:
    using ArgsType = cmd_pkg::CDBGKargVctrl;

protected:
    using MNT = monitor_pkg::CPacketMonitor;
    using MNT_FLAG = MNT::E_MNT_FLAG;
    using MNT_ERROR = MNT::E_MNT_ERROR;

public:
    CProcValve(void);

    ~CProcValve(void);

    // Making packet to be send
    std::shared_ptr<PacketType> make_packet_for_dbgcmd(std::shared_ptr<CMDDebug> &cmd) override;

    // Register result of sending packet made by 'make_packet_for_dbgcmd'.
    void register_sent_msg(uint32_t msg_id, bool result) override;

protected:
    // Thread core-routine to update data.
    bool data_update(TaskType type, std::shared_ptr<CMDType> cmd) override;

    bool register_packet(std::shared_ptr<PacketType> &packet, CMDType::FlagType flag);

    bool create_custom_threads(void) override;

    void destroy_custom_threads(void) override;

private:
    void flag_validation_check(uint32_t msg_id, CMDType::FlagType cmd_flag, std::shared_ptr<CMDType> &cmd);

    void set_flag(uint32_t msg_id, CMDType::FlagType cmd_flag);

    bool removeable_from_monitor(uint32_t msg_id);

    bool check_service_available(std::string dest_app_name);

    std::shared_ptr<CMDType> create_valve_cmd(std::shared_ptr<ArgsType> &args);

    std::list<std::string>& make_rawdata(std::shared_ptr<CMDDebug> &cmd, 
                                         std::list<std::string> &event_cmds,
                                         std::list<std::string> &period_cmds,
                                         std::string &rawdata);

    bool dbgk_cmd_insert(std::shared_ptr<CMDDebug> &cmd, 
                         std::list<std::string> &event_cmds,
                         std::list<std::string> &period_cmds);

    void dbgk_cmds_insert(std::list<std::string> &event_cmds, 
                          std::list<std::string> &period_cmds);

    void dbgk_cmds_load( std::list<std::string> &cmds_list, 
                         bool mid_breakable, 
                         bool removeable=true);

    int run_periodical_file_manager(void);

    void file_managing(std::string &period_cmds_file_path, std::string &event_cmds_file_path);

    bool read_file(std::string &file_path, std::list<std::string> &cmd_list);

    bool write_file(std::string &file_path, std::list<std::string> &cmd_list);

private:
    // Packet Monitoring-data
    MNT _monitor_;

    std::thread _runner_filemanager_;

    std::string _root_dbgk_cmd_;

};


}

#endif // _C_PROCESS_VALVE_H_