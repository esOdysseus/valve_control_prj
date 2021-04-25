#include <cassert>
#include <iostream>

#include <stdio.h>

#include <logger.h>
#include <unistd.h>
#include <CValveCTRL.h>
#include <IProtocolInf.h>
#include <CException.h>

using namespace std;

namespace valve_pkg {


/*********************************
 * Definition of Public Function.
 */
CValveCTRL::CValveCTRL(CValveCTRL::CommHandler handler){
    LOGD("Called.");

    if( init_gpio_root() == false ) {
        LOGERR("Setting of GPIO-Root path is failed.");
        throw CException(E_ERROR::E_ERR_FAIL_INITE_GPIO_ROOT);
    }

    _is_continue_ = false;
    _h_communicator_ = std::forward<CommHandler>(handler);
    _cmd_list_.clear();
    _myself_name_ = _h_communicator_->get_app_id();
    assert(_myself_name_.empty() == false);
    _server_id_ = "alias_udp_01";
    set_state(E_STATE::E_NO_STATE);
}

CValveCTRL::~CValveCTRL(void) {
    LOGD("Called.");
    soft_exit();
    LOGD("Terminated.");
}

void CValveCTRL::soft_exit(void) {
    LOGD("Called.");

    if( _is_continue_ ) {
        LOGD("Try to destroy threads...");
        destroy_threads();
    }
    _h_communicator_.reset();
    _cmd_list_.clear();
    _myself_name_.clear();
    _server_id_.clear();
    set_state(E_STATE::E_NO_STATE);
}

// This-APP was normal-ready/quit to communicate between VMs.
void CValveCTRL::cb_initialization(enum_c::ProviderType provider_type, bool flag_init) {
    LOGD("Called. arg(flag_init=%d)", flag_init);

    try {
        if( _is_continue_ == false) {
            _is_continue_ = create_threads();
        }

        if( _is_continue_ == false ) {
            destroy_threads();
            throw CException(E_ERR_FAIL_CREATING_THREAD);
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }
}

// This-APP was exit from communication-session, abnormally.
void CValveCTRL::cb_abnormally_quit(const std::exception &e) {
    LOGW("Called. arg(error=%s)", e.what());
}

// Client was connected.
void CValveCTRL::cb_connected(std::string client_id, bool flag_connect) {
    LOGD("Called. arg(flag_connect=%d)", flag_connect);
}

// We receved a message from client_id.
void CValveCTRL::cb_receive_msg_handle(std::string client_id, std::shared_ptr<payload::CPayload> payload) {
    LOGD("Called.");

    ssize_t d_size = 0;
    std::shared_ptr<CMDType> ack;
    size_t data_size = 0;
    std::shared_ptr<CMDType> valve_cmd = std::make_shared<CMDType>(client_id);
    const void* data = payload->get_payload(data_size);

    cout << "************************************" << endl;
    cout << "* 1. Server-ID : " << client_id << endl;
    cout << "* 2. CPayload-Name : " << payload->get_name() << endl;
    cout << "* 3. payload-size : " << data_size << endl;
    cout << "* 4. payload : " << (const char*)data << endl;
    cout << "************************************" << endl;

    // Parsing of received-CMD.
    if( valve_cmd->decode(data, (ssize_t)data_size) == true ) {
        // Insert CMD to list.
        if (insert_new_cmd(valve_cmd) == true) {
            if( (bool)(valve_cmd->get_flag(E_FLAG::E_FLAG_REQUIRE_ACK)) ) {
                // If valve_cmd require ACK, then send ACK message.
                LOGD("Try Sending ACK msg of ID(%d).", valve_cmd->get_id());
                send_simple(valve_cmd->get_from(), E_FLAG::E_FLAG_ACK_MSG, valve_cmd->get_id());
            }
        }
        else {
            LOGERR("Inserting new-cmd to list is failed. Please check it.");
        }
    }
}

/************************************
 * Definition of Private-Function.
 */
bool CValveCTRL::init_gpio_root(void) {
    const char* VALVE_GPIO_ROOT = getenv("VALVE_GPIO_ROOT");
    if( VALVE_GPIO_ROOT == NULL ) {
        return false;
    }

    _gpio_root_path_ = VALVE_GPIO_ROOT;
    LOGD("GPIO_ROOT_PATH=%s", _gpio_root_path_.c_str());
    return true;
}

bool CValveCTRL::create_threads(void) {
    _is_continue_ = true;

    this->_runner_exe_cmd_ = std::thread(&CValveCTRL::run_cmd_execute, this);
    if ( this->_runner_exe_cmd_.joinable() == false ) {
        LOGW("run_cmd_execute thread creating is failed.");
        _is_continue_ = false;
        set_state(E_STATE::E_STATE_THR_CMD, 0);
    }

    this->_runner_keepalive_ = std::thread(&CValveCTRL::run_keepalive, this, _server_id_);
    if ( this->_runner_keepalive_.joinable() == false ) {
        LOGW("run_keepalive thread creating is failed.");
        _is_continue_ = false;
        set_state(E_STATE::E_STATE_THR_KEEPALIVE, 0);
    }

    return _is_continue_;
}

void CValveCTRL::destroy_threads(void) {
    _is_continue_ = false;

    // Destroy of POWER-off-threads for Each-valve.
    for(int i=0; i < CMDType::E_TARGET::E_VALVE_CNT; i++) {
        std::thread &h_thread = _runner_valve_pwroff_[i];
        if( h_thread.joinable() == true ) {
            h_thread.join();
        }
    }

    // Destroy of CMD-Execute thread.
    if(_runner_exe_cmd_.joinable() == true) {
        _runner_exe_cmd_.join();
        set_state(E_STATE::E_STATE_THR_CMD, 0);
    }

    // Destroy of KEEP-ALIVE Sending thread.
    if(_runner_keepalive_.joinable() == true) {
        _runner_keepalive_.join();
        set_state(E_STATE::E_STATE_THR_KEEPALIVE, 0);
    }
}

bool CValveCTRL::insert_new_cmd(std::shared_ptr<CMDType> cmd) {
    CMDlistType::iterator itor;
    E_CMPTIME state;
    assert( cmd.get() != NULL );
    assert( cmd->parsing_complet() == true );

    try {
        std::lock_guard<std::mutex> guard(_mtx_cmd_list_);
        itor = _cmd_list_.begin();

        while( itor != _cmd_list_.end() ) {
            // search position in _cmd_list_.
            state = (*(itor))->check_with_another(cmd.get(), 0.0);
            assert( state != E_CMPTIME::E_CMPTIME_UNKNOWN);
            if ( state == E_CMPTIME::E_CMPTIME_OVER )
                break;
        }

        // insert cmd to list.
        _cmd_list_.insert(itor, cmd);
        return true;
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
    
    return false;
}

std::shared_ptr<CValveCTRL::CMDlistType> CValveCTRL::try_task_decision(void) {
    // Search Task-List to do task.
    E_CMPTIME cmd_time_state = E_CMPTIME::E_CMPTIME_UNKNOWN;
    std::shared_ptr<CMDType> valve_cmd;
    CMDlistType::iterator itor;
    CMDlistType::iterator itor_past;
    std::shared_ptr<CMDlistType> cmds_to_exe = std::make_shared<CMDlistType>();

    std::lock_guard<std::mutex> guard(_mtx_cmd_list_);
    itor = _cmd_list_.begin();

    while( itor != _cmd_list_.end() ) {
        valve_cmd.reset();
        valve_cmd = *itor;

        cmd_time_state = valve_cmd->check_with_curtime();
        if ( cmd_time_state == E_CMPTIME::E_CMPTIME_EQUAL ||
             cmd_time_state == E_CMPTIME::E_CMPTIME_UNDER ) {
            cmds_to_exe->push_back(valve_cmd);
            itor_past = itor;
            itor++;
            _cmd_list_.erase(itor_past);
        }
        else {
            break;
        }
    }

    return cmds_to_exe;
}

void CValveCTRL::execute_cmds(std::shared_ptr<CMDlistType> &cmds) {
    std::shared_ptr<CMDType> valve_cmd;
    CMDlistType::iterator itor = cmds->begin();

    while( itor != cmds->end() ) {
        valve_cmd.reset();
        valve_cmd = *itor;

        // Wait pre-runned thread (valve power disable)
        LOGD("Try to join of pre-runned thread.");
        std::thread &h_thread = _runner_valve_pwroff_[valve_cmd->get_what()];
        if (h_thread.joinable()) {
            h_thread.join();
        }

        // Act valve-command with power enable.
        LOGD("Power Enable & Act Valve-cmd.");
        if( execute_valve_cmd(valve_cmd, E_PWR::E_PWR_ENABLE) != true ) {
            LOGERR("Executing valve-command is failed.");
        }

        // Stop action of valve-command by power disable.
        LOGD("Try creating thread.");
        h_thread = std::thread([this](std::shared_ptr<CMDType> cmd){
            LOGD("Success Creating thread.");
            uint32_t wait_sec = 1;
            std::string t_how = cmd->get_how();
            if ( t_how == CMDType::OPEN ) {
                wait_sec = WAITSEC_VALVE_OPEN;
            }
            else if ( t_how == CMDType::CLOSE ) {
                wait_sec = WAITSEC_VALVE_CLOSE;
            }
            else {
                LOGERR("Not Supported How(%s).", t_how.data());
                wait_sec = 1;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(wait_sec));

            if( execute_valve_cmd(cmd, E_PWR::E_PWR_DISENABLE) != true ) {
                LOGERR("Executing valve-command is failed.");
            }
        }, valve_cmd);

        itor++;
    }
}

/** Valve Open/Close routin.*/
bool CValveCTRL::execute_valve_cmd(std::shared_ptr<CMDType> &valve_cmd, E_PWR power) {
    bool result = false;
    std::string t_gpio;
    int t_gpio_value = 1;

    assert( valve_cmd.get() != NULL );
    assert( valve_cmd->parsing_complet() == true );

    try{
        cout << "************ POWER = " << power << " **************" << endl;
        cout << "who=" << valve_cmd->get_who() << endl;
        cout << "when=" << valve_cmd->print_when() << endl;
        cout << "current=" << valve_cmd->print_cur_time() << endl;
        cout << "CMP:time=" << valve_cmd->check_with_curtime() << endl;
        cout << "what=" << valve_cmd->get_what() << endl;
        cout << "how=" << valve_cmd->get_how() << endl;
        cout << "why=" << valve_cmd->get_why() << endl;
        cout << "************************************" << endl;
        
        // get gpio-path for control valve.
        t_gpio = get_gpio_path(valve_cmd);
        
        // get gpio-pin value according to power-state.
        switch(power) {
        case E_PWR::E_PWR_ENABLE:
            t_gpio_value = 0;
            break;
        case E_PWR::E_PWR_DISENABLE:
            t_gpio_value = 1;
            break;
        }

        // write GPIO with value.
        if( (result = valve_set(t_gpio, t_gpio_value))==true && power==E_PWR::E_PWR_ENABLE ) {
            // if need it, then send ACT_DONE message to server.
            if( conditional_send_actdone(valve_cmd) != true ) {
                LOGERR("Can not send ACT-Done message.");
                throw CException(E_ERROR::E_ERR_FAIL_SENDING_ACT_DONE);
            }
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        // throw dynamic_cast<const CException&>(e);
    }

    return result;
}

std::string CValveCTRL::get_gpio_path(std::shared_ptr<CMDType> &valve_cmd) {
    int gpio_index = -1;
    std::string t_how;
    std::string t_gpio = _gpio_root_path_ + "/";

    try {
        // deal with 'how'
        t_how = valve_cmd->get_how();
        if ( t_how == CMDType::OPEN ) {
            gpio_index = 0;
        }
        else if ( t_how == CMDType::CLOSE ) {
            gpio_index = CMDType::E_TARGET::E_VALVE_CNT;
        }
        else {
            LOGERR("Not supported Type of How-value.(%s)", t_how.c_str());
            throw CException(E_ERR_NOT_SUPPORTED_HOW);
        }

        // deal with 'what'
        gpio_index += valve_cmd->get_what();
        switch(gpio_index){
        /** Valve 01 */
        case E_GPIO::E_VALVE_LEFT_01_OPEN:
            t_gpio += "gpio107/value";      // PD11 pin-out
            break;
        case E_GPIO::E_VALVE_LEFT_01_CLOSE:
            t_gpio += "gpio110/value";      // PD14 pin-out
            break;
        /** Valve 02 */
        case E_GPIO::E_VALVE_LEFT_02_OPEN:
            t_gpio += "gpio13/value";       // PA13 pin-out
            break;
        case E_GPIO::E_VALVE_LEFT_02_CLOSE:
            t_gpio += "gpio14/value";       // PA14 pin-out
            break;
        /** Valve 03 */
        case E_GPIO::E_VALVE_LEFT_03_OPEN:
            t_gpio += "gpio15/value";       // PA15 pin-out
            break;
        case E_GPIO::E_VALVE_LEFT_03_CLOSE:
            t_gpio += "gpio16/value";       // PA16 pin-out
            break;
        /** Valve 04 */
        case E_GPIO::E_VALVE_LEFT_04_OPEN:
            t_gpio += "gpio18/value";       // PA18 pin-out
            break;
        case E_GPIO::E_VALVE_LEFT_04_CLOSE:
            t_gpio += "gpio19/value";       // PA19 pin-out
            break;
        default:
            LOGW("Not supported Target-GPIO.(%d)", valve_cmd->get_what());
            throw CException(E_ERR_NOT_SUPPORTED_WHAT);
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        t_gpio.clear();
        throw dynamic_cast<const CException&>(e);
    }

    return t_gpio;
}

bool CValveCTRL::valve_set(std::string gpio_path, int value) {
    LOGD("Called.");
    int sys_output = -1;
    char *buf = NULL;
    bool result = false;
    ssize_t buf_size = 0;
    assert( gpio_path.empty() == false );
    assert( value == 0 || value == 1 );
    
    // run command
    if ( system(NULL) ) {
        assert( (buf_size = 10+gpio_path.length()+1) > 11 );
        assert( (buf = new char[buf_size]) != NULL );

        assert( snprintf(buf, buf_size, "echo %d > %s", value, gpio_path.c_str()) > 0 );
        LOGD("command: [%s]", buf);
        sys_output = system(buf);
        switch( sys_output ) {    // run command as bash shell
        case -1:
            LOGW("There is Error when you run command in system.");
            throw CException(E_ERROR::E_ERR_FAIL_RUNNING_CMD);
            break;
        case 127:
            LOGW("/bin/sh invoking is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_INVOKING_SHELL);
        default:
            LOGI("Output of system(%s) is [%d].", buf, sys_output);
            result = true;
        }
    }
    else {
        LOGW("Command Processor is not available.");
        // delete heap memory.
        if( buf ) {
            delete[] buf;
            buf = NULL;
        }
        throw CException(E_ERROR::E_ERR_FAIL_CHECKING_CMD_PROC);
    }

    // delete heap memory.
    if( buf ) {
        delete[] buf;
        buf = NULL;
    }

    return result;
}

/********************************
 * Definition of Thread-Routin
 */
int CValveCTRL::run_cmd_execute(void) {
    LOGD("Called.");
    int send_count = 0;
    std::shared_ptr<CMDlistType> cmds;

    set_state(E_STATE::E_STATE_THR_CMD, 1);

    while(_is_continue_) {
        cmds.reset();

        // Check Current-Tasks & execute thoese.
        cmds = try_task_decision();
        if ( cmds->size() > 0 ) {
            execute_cmds(cmds);
            cmds.reset();
        }

        // wait 300 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    set_state(E_STATE::E_STATE_THR_CMD, 0);
}

int CValveCTRL::run_keepalive(std::string target_id) {
    LOGD("Called. target-id=%s", target_id.c_str());

    set_state(E_STATE::E_STATE_THR_KEEPALIVE, 1);

    while(_is_continue_) {
        try {
            if( send_simple(target_id, E_FLAG::E_FLAG_KEEPALIVE) == false ) {
                LOGERR("Sending Keep-Alive message is failed.");
            }

            // wait 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        catch (const std::exception &e) {
            LOGERR("%s", e.what());
        }
    }

    set_state(E_STATE::E_STATE_THR_KEEPALIVE, 0);
}

bool CValveCTRL::conditional_send_actdone(std::shared_ptr<CMDType> &valve_cmd) {
    try {
        if( (bool)(valve_cmd->get_flag(E_FLAG::E_FLAG_REQUIRE_ACT)) == true ) {
            LOGD("Try Sending ACTION-DONE msg of ID(%d).", valve_cmd->get_id());
            return send_simple(valve_cmd->get_from(), 
                               E_FLAG::E_FLAG_ACTION_DONE, 
                               valve_cmd->get_id());
        }
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }

    return false;
}

bool CValveCTRL::send_simple(std::string target_id, E_FLAG flag, unsigned long msg_id) {
    void* data = NULL;
    ssize_t d_size = 0;
    std::shared_ptr<CMDType> simple_msg;
    std::shared_ptr<payload::CPayload> new_payload;
    assert( flag == E_FLAG::E_FLAG_ACK_MSG || 
            flag == E_FLAG::E_FLAG_ACTION_DONE || 
            flag == E_FLAG::E_FLAG_KEEPALIVE );
    
    try {
        if( flag != E_FLAG::E_FLAG_KEEPALIVE ) {
            assert( msg_id > 0 );
        }
        
        new_payload = _h_communicator_->create_payload();
        simple_msg = std::make_shared<CMDType>(_myself_name_, target_id, flag);

        // Check current state & set it to cmd.
        set_state_of_cmd(simple_msg);

        // Set current-time.
        simple_msg->set_when();

        // Set message-ID.
        simple_msg->set_id(msg_id);

        // Encode cmd to packet.
        if( (data = simple_msg->encode(d_size)) == NULL ) {
            LOGERR("Encoding message is failed. Please check it.");
            throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
        }

        // Send message.
        assert( new_payload->set_payload(data, d_size) == true );
        assert( _h_communicator_->send(target_id, new_payload) == true );
        return true;
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        if( data != NULL ) {
            delete[] data;
            data = NULL;
        }
        throw dynamic_cast<const CException&>(e);
    }

    if( data != NULL ) {
        delete[] data;
        data = NULL;
    }
    return false;
}

void CValveCTRL::set_state_of_cmd(std::shared_ptr<CMDType> &cmd) {
    StateType cur_state = E_STATE::E_NO_STATE;

    // Check current state.
    cur_state = get_state(E_STATE::E_STATE_ALL);
    cmd->set_state(cur_state);

    // Check ab-normal state & set flag
    if ( cur_state & (E_STATE_OUT_OF_SERVICE|E_STATE_OCCURE_ERROR) ) {
        cmd->set_flag(E_FLAG::E_FLAG_STATE_ERROR, 1);
    }
}

void CValveCTRL::set_state(E_STATE pos, StateType value) {
    int shift_cnt = 0;

    if ( pos == E_STATE::E_NO_STATE ) {
        _state_ = value;
    }
    else {
        // Assumption : pos is continuous-bitmask.
        while( ((1<<shift_cnt) & pos) == 0 ) {
            shift_cnt++;
            assert( shift_cnt < (sizeof(StateType)*8) );
        }

        _state_ = (_state_ & (~pos)) | ((value << shift_cnt) & pos);
    }
}

CValveCTRL::StateType CValveCTRL::get_state(E_STATE pos) {
    assert( pos != E_STATE::E_NO_STATE);
    return _state_ & pos;
}


}   // namespace valve_pkg
