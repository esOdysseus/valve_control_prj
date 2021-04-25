#ifndef C_APP_TEST_H_
#define C_APP_TEST_H_

#include <list>
#include <string>
#include <thread>
#include <memory>

#include <IAppInf.h>
#include <CVctrlCMD.h>

namespace valve_pkg {

typedef enum E_STATE {
    E_NO_STATE              = 0x0000,
    E_STATE_THR_GPS         = 0x0001,
    E_STATE_THR_CMD         = 0x0002,
    E_STATE_THR_KEEPALIVE   = 0x0004,
    E_STATE_OUT_OF_SERVICE  = 0x0008,
    E_STATE_OCCURE_ERROR    = 0x0010,
    E_STATE_ALL             = 0xFFFF
} E_STATE;

class CValveCTRL {
public:
    using StateType = uint16_t;
    using CommHandler = std::shared_ptr<ICommunicator>;
    using CMDType = CVctrlCMD;
    using CMDlistType = std::list<std::shared_ptr<CMDType>>;
    using E_PWR = enum E_PWR {
        E_PWR_DISENABLE = 0,
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

public:
    CValveCTRL(CommHandler handler);

    ~CValveCTRL(void);

    void soft_exit(void);

    void cb_initialization(enum_c::ProviderType provider_type, bool flag_init);

    void cb_connected(std::string client_id, bool flag_connect);

    // Control valves according to command of Center-Brain.
    void cb_receive_msg_handle(std::string client_id, std::shared_ptr<payload::CPayload> payload);

    void cb_abnormally_quit(const std::exception &e);

private:
    bool init_gpio_root(void);

    /** Functions for life-cycle of Thread */
    bool create_threads(void);

    void destroy_threads(void);

    /** Thread-routin */
    int run_cmd_execute(void); // Execute command routin.

    int run_keepalive(std::string server_id); // Keep-Alive send routin.

    /** Functions for Thread */
    bool conditional_send_actdone(std::shared_ptr<CMDType> &valve_cmd);

    bool send_simple(std::string target_id, E_FLAG flag, unsigned long msg_id=0);

    void set_state_of_cmd(std::shared_ptr<CMDType> &cmd);

    void set_state(E_STATE pos, StateType value=0);

    StateType get_state(E_STATE pos);

    /** Functions with regard to CMD */
    bool insert_new_cmd(std::shared_ptr<CMDType> cmd);

    std::shared_ptr<CMDlistType> try_task_decision(void);

    void execute_cmds(std::shared_ptr<CMDlistType> &cmds);

    bool execute_valve_cmd(std::shared_ptr<CMDType> &valve_cmd, E_PWR power);

    std::string get_gpio_path(std::shared_ptr<CMDType> &valve_cmd);

    bool valve_set(std::string gpio_path, int value);

private:
    bool _is_continue_;       // Thread continue-flag.
    
    std::thread _runner_exe_cmd_; // Periodically, Thread that is charge of deciding & executing for received CMD.

    std::thread _runner_keepalive_; // Periodically, Thread that is charge of sending Keep-Alive message.

    std::thread _runner_valve_pwroff_[CMDType::E_TARGET::E_VALVE_CNT];

    StateType _state_;

    CommHandler _h_communicator_; // Communicator-handler.

    CMDlistType _cmd_list_;     // cmd encode/decode for valve-controling.

    std::mutex _mtx_cmd_list_;

    std::string _myself_name_;

    std::string _server_id_;

    std::string _gpio_root_path_;

    static constexpr uint32_t WAITSEC_VALVE_OPEN = 25;
    static constexpr uint32_t WAITSEC_VALVE_CLOSE = 25;

};

}   // namespace valve_pkg


#endif // C_APP_TEST_H_
