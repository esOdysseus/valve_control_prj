#ifndef _VALVE_COMMUNICATOR_H_
#define _VALVE_COMMUNICATOR_H_

#include <list>
#include <string>
#include <thread>
#include <memory>

#include <ICommunicator.h>
#include <CCommand.h>
#include <CController.h>
#include <Common.h>

namespace valve_pkg {


class CCommunicator {
public:
    using CommHandler = std::shared_ptr<ICommunicator>;
    using CMDType = CCommand;

public:
    CCommunicator(CommHandler handler, std::string& myapp_path, std::string& mypvd_id);

    ~CCommunicator(void);

    void soft_exit(void);

    void cb_initialization(enum_c::ProviderType provider_type, bool flag_init);

    void cb_connected(std::string app_path, std::string pvd_id, bool flag_connect);

    // Control valves according to command of Center-Brain.
    void cb_receive_msg_handle(std::string app_path, std::string pvd_id, std::shared_ptr<payload::CPayload> payload);

    void cb_abnormally_quit(const std::exception &e);

    /** Functions for Thread */
    bool conditional_send_actdone(std::shared_ptr<CMDType> &valve_cmd);

    void set_state(E_STATE pos, StateType value);

    StateType get_state(E_STATE pos);

private:
    bool create_threads(void);
    
    void destroy_threads(void);

    int run_keepalive(alias::CAlias target); // Keep-Alive send routin.

    bool send_simple(alias::CAlias target, E_FLAG flag, unsigned long msg_id=0);

    void set_state_of_cmd(std::shared_ptr<CMDType> &cmd);

private:
    bool _is_continue_;       // Thread continue-flag.

    CController _ctrller_;   // Valve-Controller class.

    CommHandler _h_communicator_; // Communicator-handler.

    alias::CAlias _myself_;

    alias::CAlias _peer_;

    std::thread _runner_keepalive_; // Periodically, Thread that is charge of sending Keep-Alive message.

    StateType _state_;

};


}   // namespace valve_pkg


#endif // _VALVE_COMMUNICATOR_H_
