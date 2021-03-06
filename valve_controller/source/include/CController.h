#ifndef _VALVE_CONTROLLER_H_
#define _VALVE_CONTROLLER_H_

#include <list>
#include <string>
#include <thread>
#include <memory>
#include <mutex>

#include <CuCMD/CuCMD.h>
#include <Common.h>
#include <CuCMD/MCommunicator.h>

namespace valve_pkg {


class CController {
public:
    using E_STATE = common::E_STATE;
    using StateType = common::StateType;
    using CMDType = cmd::CuCMD;
    using CMDlistType = std::list<std::shared_ptr<CMDType>>;
    using E_PWR = enum E_PWR {
        E_PWR_DISABLE = 0,
        E_PWR_ENABLE = 1
    };
    using E_GPIO = enum E_GPIO {
        E_VALVE_LEFT_01_OPEN = 0,
        E_VALVE_LEFT_02_OPEN = 1,
        E_VALVE_LEFT_03_OPEN = 2,
        E_VALVE_LEFT_04_OPEN = 3,
        E_VALVE_LEFT_01_CLOSE = 4,
        E_VALVE_LEFT_02_CLOSE = 5,
        E_VALVE_LEFT_03_CLOSE = 6,
        E_VALVE_LEFT_04_CLOSE = 7
    };

private:
    using Tvalve_method = ::principle::Tvalve_method;
    using Tdb_method = ::principle::Tdb_method;

    static constexpr const char* OPEN = "open";
    static constexpr const char* CLOSE = "close";
    
    using E_VALVE = enum E_VALVE {
        E_VALVE_NULL = -1,
        E_VALVE_LEFT_01 = 0,
        E_VALVE_LEFT_02 = 1,
        E_VALVE_LEFT_03 = 2,
        E_VALVE_LEFT_04 = 3,
        E_VALVE_CNT = 4
    };

public:
    CController(void);

    ~CController(void);

    void init(std::shared_ptr<::comm::MCommunicator>& comm);

    void soft_exit(void);

    /** Functions for life-cycle of Thread */
    bool create_threads(void);

    void destroy_threads(void);

    void receive_command( std::shared_ptr<cmd::ICommand>& cmd );

    void set_service_indicator( bool state );

private:
    void clear(void);

    bool init_gpio_root(void);

    /** Thread-routin */
    int run_cmd_execute(void); // Execute command routin.

    void set_state(E_STATE pos, StateType value);

    StateType get_state(E_STATE pos);

    bool push_cmd(std::shared_ptr<CMDType> cmd);

    /** Functions with regard to CMD */
    bool insert_cmd(std::shared_ptr<CMDType> cmd);

    std::shared_ptr<CMDlistType> pop_tasks(void);

    void execute_cmds(std::shared_ptr<CMDlistType> &cmds);

    bool execute_valve_cmd(std::shared_ptr<CMDType> &valve_cmd, E_PWR power);

    std::string get_gpio_path(std::shared_ptr<CMDType> &valve_cmd);

    bool set_gpio(std::string gpio_path, int value);

    CMDlistType decompose_cmd(std::shared_ptr<CMDType> cmd);

private:
    std::shared_ptr<::comm::MCommunicator> _comm_;
    
    std::shared_ptr<alias::CAlias> _m_myself_;

    bool _is_continue_;       // Thread continue-flag.
    
    std::thread _runner_exe_cmd_; // Periodically, Thread that is charge of deciding & executing for received CMD.

    std::thread _runner_valve_pwroff_[E_VALVE::E_VALVE_CNT];

    CMDlistType _cmd_list_;     // cmd encode/decode for valve-controling.

    std::mutex _mtx_cmd_list_;

    std::string _gpio_root_path_;

    static constexpr uint32_t WAITSEC_VALVE_OPEN = 25;
    static constexpr uint32_t WAITSEC_VALVE_CLOSE = 25;

};


}   // namespace valve_pkg


#endif // _VALVE_CONTROLLER_H_
