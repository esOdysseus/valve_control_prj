#include <cassert>
#include <iostream>

#include <stdio.h>

#include <logger.h>
#include <unistd.h>
#include <CController.h>
#include <IProtocolInf.h>
#include <CException.h>
#include <CCommunicator.h>
#include <time_kes.h>

using namespace std;

namespace valve_pkg {

constexpr const char* CController::OPEN;
constexpr const char* CController::CLOSE;

/*********************************
 * Definition of Public Function.
 */
CController::CController( CCommunicator* handler ){
    LOGD("Called.");
    _comm_ = handler;
    if( _comm_ == NULL ) {
        LOGERR("Communicator is NULL.");
        throw CException(E_ERROR::E_ERR_INVALID_NULL_VALUE);
    }

    if( init_gpio_root() == false ) {
        LOGERR("Setting of GPIO-Root path is failed.");
        throw CException(E_ERROR::E_ERR_FAIL_INITE_GPIO_ROOT);
    }

    _is_continue_ = false;
    _cmd_list_.clear();
    set_state(E_STATE::E_NO_STATE, 0);
}

CController::~CController(void) {
    LOGD("Called.");
    if( _comm_ != NULL ) {
        soft_exit();
        LOGD("Terminated.");
    }
}

void CController::soft_exit(void) {
    LOGD("Called.");

    if( _is_continue_ ) {
        LOGD("Try to destroy threads...");
        destroy_threads();
    }

    _cmd_list_.clear();
    set_state(E_STATE::E_NO_STATE, 0);
    _comm_ = NULL;
}

bool CController::create_threads(void) {
    _is_continue_ = true;

    this->_runner_exe_cmd_ = std::thread(&CController::run_cmd_execute, this);
    if ( this->_runner_exe_cmd_.joinable() == false ) {
        LOGW("run_cmd_execute thread creating is failed.");
        _is_continue_ = false;
        set_state(E_STATE::E_STATE_THR_CMD, 0);
    }

    return _is_continue_;
}

void CController::destroy_threads(void) {
    _is_continue_ = false;

    // Destroy of POWER-off-threads for Each-valve.
    for(int i=0; i < E_VALVE::E_VALVE_CNT; i++) {
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
}

bool CController::apply_new_cmd(std::shared_ptr<CMDType> cmd) {
    bool res = false;
    try {
        auto cmds = decompose_cmd( cmd );

        for( auto itr=cmds.begin(); itr!=cmds.end(); itr++ ) {
            res = insert_cmd( cmd );
            if( res == false ) {
                throw std::runtime_error("Inserting CMDs is failed.");
            }
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        res = false;
    }

    return res;
}


/************************************
 * Definition of Private-Function.
 */
bool CController::init_gpio_root(void) {
    const char* VALVE_GPIO_ROOT = getenv("VALVE_GPIO_ROOT");
    if( VALVE_GPIO_ROOT == NULL ) {
        return false;
    }

    _gpio_root_path_ = VALVE_GPIO_ROOT;
    LOGD("GPIO_ROOT_PATH=%s", _gpio_root_path_.c_str());
    return true;
}

std::shared_ptr<CController::CMDlistType> CController::try_task_decision(void) {
    // Search Task-List to do task.
    cmd::E_CMPTIME cmd_time_state = cmd::E_CMPTIME::E_CMPTIME_UNKNOWN;
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
        if ( cmd_time_state == cmd::E_CMPTIME::E_CMPTIME_EQUAL ||
             cmd_time_state == cmd::E_CMPTIME::E_CMPTIME_UNDER ) {
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

void CController::execute_cmds(std::shared_ptr<CMDlistType> &cmds) {
    std::shared_ptr<CMDType> valve_cmd;
    CMDlistType::iterator itor = cmds->begin();

    while( itor != cmds->end() ) {
        valve_cmd.reset();
        valve_cmd = *itor;

        try {
            // Wait pre-runned thread (valve power disable)
            LOGD("Try to join of pre-runned thread.");
            std::thread &h_thread = _runner_valve_pwroff_[valve_cmd->what().get_which()];
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
            h_thread = std::thread([this](std::shared_ptr<CMDType> cmd) {
                LOGD("Success Creating thread.");
                uint32_t wait_sec = 1;
                std::string t_how = cmd->how().get_method();
                if ( t_how == OPEN ) {
                    wait_sec = WAITSEC_VALVE_OPEN;
                }
                else if ( t_how == CLOSE ) {
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
        }
        catch ( const std::exception& e ) {
            LOGERR("%s", e.what());
        }

        itor++;
    }
}

/** Valve Open/Close routin.*/
bool CController::execute_valve_cmd(std::shared_ptr<CMDType> &valve_cmd, E_PWR power) {
    bool result = false;
    std::string t_gpio;
    int t_gpio_value = 1;

    assert( _comm_ != NULL );
    assert( valve_cmd.get() != NULL );
    assert( valve_cmd->parsing_complet() == true );

    try{
        cout << "************ POWER = " << power << " **************" << endl;
        // cout << "who=" << valve_cmd->get_who() << endl;
        cout << "send_time=" << valve_cmd->print_send_time() << endl;
        cout << "current=" << time_pkg::CTime::print_nanotime() << endl;
        cout << "CMP:time=" << valve_cmd->check_with_curtime() << endl;
        cout << "what=" << valve_cmd->what().get_type() << endl;
        cout << "how=" << valve_cmd->how().get_method() << endl;
        cout << "costtime=" << valve_cmd->how().get_costtime() << endl;
        cout << "why=" << valve_cmd->why() << endl;
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
            if( _comm_->conditional_send_actdone(valve_cmd) != true ) {
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

std::string CController::get_gpio_path(std::shared_ptr<CMDType> &valve_cmd) {
    int gpio_index = -1;
    std::string t_how;
    std::string t_gpio = _gpio_root_path_ + "/";

    try {
        // deal with 'how'
        t_how = valve_cmd->how().get_method();
        if ( t_how == OPEN ) {
            gpio_index = 0;
        }
        else if ( t_how == CLOSE ) {
            gpio_index = E_VALVE::E_VALVE_CNT;
        }
        else {
            LOGERR("Not supported Type of How-value.(%s)", t_how.c_str());
            throw CException(E_ERR_NOT_SUPPORTED_HOW);
        }

        // deal with 'what'
        gpio_index += valve_cmd->what().get_which();
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
            LOGW("Not supported Target-GPIO.(%d)", valve_cmd->what().get_which());
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

bool CController::valve_set(std::string gpio_path, int value) {
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

bool CController::insert_cmd(std::shared_ptr<CMDType> cmd) {
    CMDlistType::iterator itor;
    cmd::E_CMPTIME state;
    assert( cmd.get() != NULL );
    assert( cmd->parsing_complet() == true );

    try {
        std::lock_guard<std::mutex> guard(_mtx_cmd_list_);
        itor = _cmd_list_.begin();

        while( itor != _cmd_list_.end() ) {
            // search position in _cmd_list_.
            state = (*(itor))->check_with_another(cmd.get(), 0.0);
            assert( state != cmd::E_CMPTIME::E_CMPTIME_UNKNOWN);
            if ( state == cmd::E_CMPTIME::E_CMPTIME_OVER )
                break;
            
            itor++;
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

CController::CMDlistType CController::decompose_cmd(std::shared_ptr<CMDType> cmd) {
    CMDlistType cmds;
    if( cmd.get() == NULL ) {
        throw std::runtime_error("CMD is empty.");
    }

    try {
        auto method = cmd->how().get_method();
        double costtime = cmd->how().get_costtime();

        // Check invalid command.
        if ( method == OPEN && costtime <= (double)WAITSEC_VALVE_OPEN ) {
            std::string err = "Invalid-CMD is discarded. (method: Open, but costtime <= " + std::to_string(WAITSEC_VALVE_OPEN) + ")";
            throw std::invalid_argument(err);
        }

        // Insert commands
        cmds.push_back( cmd );
        if( method == OPEN ) {
            // sub_cmd의 method, 실행시간 수정. (Close)
            auto cmd_when = cmd->when();
            double close_time = cmd_when.get_start_time() + cmd->how().get_costtime();
            auto sub_cmd = std::make_shared<CMDType>( *cmd );
            if( sub_cmd.get() == NULL ) {
                throw std::runtime_error("sub_cmd memory-allocation is failed.");
            }
            
            sub_cmd->set_how( CLOSE );
            sub_cmd->set_when( cmd_when.get_type(), close_time );
            cmds.push_back( sub_cmd );
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return cmds;
}


/********************************
 * Definition of Thread-Routin
 */
int CController::run_cmd_execute(void) {
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

void CController::set_state(E_STATE pos, StateType value) {
    if( _comm_ == NULL ) {
        LOGERR("Communicator is NULL.");
        throw CException(E_ERROR::E_ERR_INVALID_NULL_VALUE);
    }

    _comm_->set_state(pos, value);
}

StateType CController::get_state(E_STATE pos) {
    if( _comm_ == NULL ) {
        LOGERR("Communicator is NULL.");
        throw CException(E_ERROR::E_ERR_INVALID_NULL_VALUE);
    }

    return _comm_->get_state(pos);
}


}   // namespace valve_pkg
