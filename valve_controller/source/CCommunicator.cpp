#include <cassert>
#include <iostream>

#include <stdio.h>

#include <logger.h>
#include <unistd.h>
#include <CCommunicator.h>
#include <IProtocolInf.h>
#include <IAliasPVD.h>
#include <CException.h>

using namespace std;

namespace valve_pkg {


/*********************************
 * Definition of Public Function.
 */
CCommunicator::CCommunicator( CCommunicator::CommHandler handler, std::string& myapp_path, std::string& mypvd_id )
: _ctrller_(this), _myself_(myapp_path, mypvd_id), _peer_("CMD-Scheduler", "cmd_sender") {
    LOGD("Called.");
    _is_continue_ = false;

    _h_communicator_ = std::forward<CommHandler>(handler);
    if( _h_communicator_.get() == NULL ) {
        LOGERR("Communicator is NULL.");
        throw CException(E_ERROR::E_ERR_INVALID_NULL_VALUE);
    }

    if( _h_communicator_->get_app_id() != myapp_path || _h_communicator_->get_provider_id() != mypvd_id ) {
        LOGERR("app_path or pvd_id is miss-matched.");
        throw CException(E_ERROR::E_ERR_INVALID_VALUE);
    }

    set_state(E_STATE::E_NO_STATE, 0);
}

CCommunicator::~CCommunicator(void) {
    LOGD("Called.");
    soft_exit();
    LOGD("Terminated.");
}

void CCommunicator::soft_exit(void) {
    LOGD("Called.");

    if( _is_continue_ ) {
        LOGD("Try to destroy threads...");
        destroy_threads();
    }

    _ctrller_.soft_exit();
    _h_communicator_.reset();
    set_state(E_STATE::E_NO_STATE, 0);
}

// This-APP was normal-ready/quit to communicate between VMs.
void CCommunicator::cb_initialization(enum_c::ProviderType provider_type, bool flag_init) {
    LOGD("Called. arg(flag_init=%d)", flag_init);

    try {
        if( _is_continue_ == false) {
            _is_continue_ = create_threads();
        }

        if( _is_continue_ == false ) {
            destroy_threads();
            throw CException(E_ERROR::E_ERR_FAIL_CREATING_THREAD);
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }
}

// Client was connected.
void CCommunicator::cb_connected(std::string app_path, std::string pvd_id, bool flag_connect) {
    LOGD("Peer(%u/%u) is connected as (flag_connect=%d)", app_path.data(), pvd_id.data(), flag_connect);
}

// We receved a message from client_id.
void CCommunicator::cb_receive_msg_handle(std::string app_path, std::string pvd_id, 
                                          std::shared_ptr<payload::CPayload> payload) {
    LOGD("Called.");

    std::string from_full_path = cf_alias::IAlias::make_full_path(app_path, pvd_id);
    std::shared_ptr<CMDType> valve_cmd = std::make_shared<CMDType>( app_path, pvd_id );
    std::shared_ptr<IProtocolInf> protocol = payload->get(CMDType::PROTOCOL_NAME);

    cout << "************************************" << endl;
    cout << "* 1. Peer-ID : " << from_full_path << endl;
    cout << "* 2. CPayload-Name : " << payload->get_name() << endl;
    cout << "************************************" << endl;

    // Parsing of received-CMD.
    if( valve_cmd->decode( protocol ) == true ) {
        // Insert CMD to list.
        if (_ctrller_.apply_new_cmd(valve_cmd) == true) {
            if( (bool)(valve_cmd->get_flag(E_FLAG::E_FLAG_REQUIRE_ACK)) ) {
                // If valve_cmd require ACK, then send ACK message.
                LOGD("Try Sending ACK msg of ID(%d).", valve_cmd->get_id());
                send_simple( valve_cmd->get_from(), E_FLAG::E_FLAG_ACK_MSG, valve_cmd->get_id());
            }
        }
        else {
            LOGERR("Inserting new-cmd to list is failed. Please check it.");
        }
    }
}

// This-APP was exit from communication-session, abnormally.
void CCommunicator::cb_abnormally_quit(const std::exception &e) {
    LOGW("Called. arg(error=%s)", e.what());
}

bool CCommunicator::conditional_send_act_start(std::shared_ptr<CMDType> &valve_cmd) {
    try {
        if( (bool)(valve_cmd->get_flag(E_FLAG::E_FLAG_REQUIRE_ACT)) == true ) {
            LOGD("Try Sending ACTION-START msg of ID(%d).", valve_cmd->get_id());
            return send_simple(valve_cmd->get_from(), 
                               E_FLAG::E_FLAG_ACTION_START, 
                               valve_cmd->get_id());
        }
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }

    return false;
}

void CCommunicator::set_state(E_STATE pos, StateType value) {
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

common::StateType CCommunicator::get_state(E_STATE pos) {
    assert( pos != E_STATE::E_NO_STATE);
    return _state_ & pos;
}

/************************************
 * Definition of Private-Function.
 */
bool CCommunicator::create_threads(void) {
    _is_continue_ = true;
    _ctrller_.create_threads();

    this->_runner_keepalive_ = std::thread(&CCommunicator::run_keepalive, this, _peer_);
    if ( this->_runner_keepalive_.joinable() == false ) {
        LOGW("run_keepalive thread creating is failed.");
        _is_continue_ = false;
        set_state(E_STATE::E_STATE_THR_KEEPALIVE, 0);
    }

    return _is_continue_;
}

void CCommunicator::destroy_threads(void) {
    _is_continue_ = false;
    _ctrller_.destroy_threads();

    // Destroy of KEEP-ALIVE Sending thread.
    if(_runner_keepalive_.joinable() == true) {
        _runner_keepalive_.join();
        set_state(E_STATE::E_STATE_THR_KEEPALIVE, 0);
    }
}

/********************************
 * Definition of Thread-Routin
 */
int CCommunicator::run_keepalive(alias::CAlias target) {
    LOGD("target-id=%s/%s", target.app_path.data(), target.pvd_id.data());

    set_state(E_STATE::E_STATE_THR_KEEPALIVE, 1);

    while(_is_continue_) {
        try {
            if( send_simple(target, E_FLAG::E_FLAG_KEEPALIVE) == false ) {
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

bool CCommunicator::send_simple(alias::CAlias target, E_FLAG flag, unsigned long msg_id) {
    std::shared_ptr<CMDType> simple_msg;
    std::shared_ptr<payload::CPayload> new_payload;
    assert( flag == E_FLAG::E_FLAG_ACK_MSG || 
            flag == E_FLAG::E_FLAG_ACTION_START || 
            flag == E_FLAG::E_FLAG_KEEPALIVE );
    
    try {
        if( flag != E_FLAG::E_FLAG_KEEPALIVE ) {
            assert( msg_id > 0 );
        }
        
        LOGD("Is empty of MySelf? (%u)", _myself_.empty());
        simple_msg = std::make_shared<CMDType>(_myself_, flag);
        // Check current state & set it to cmd.
        set_state_of_cmd(simple_msg);
        // Set message-ID.
        simple_msg->set_id(msg_id);

        // Encode cmd to packet.
        new_payload = simple_msg->encode( _h_communicator_ );
        if( new_payload.get() == NULL ) {
            LOGERR("Encoding message is failed. Please check it.");
            throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
        }

        // Send message.
        assert( _h_communicator_->send(target.app_path, target.pvd_id, new_payload) == true );
        return true;
    }
    catch ( const CException& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

void CCommunicator::set_state_of_cmd(std::shared_ptr<CMDType> &cmd) {
    StateType cur_state = E_STATE::E_NO_STATE;

    // Check current state.
    cur_state = get_state(E_STATE::E_STATE_ALL);
    cmd->set_state(cur_state);

    // // Check ab-normal state & set flag
    // if ( cur_state & (E_STATE::E_STATE_OUT_OF_SERVICE | E_STATE::E_STATE_OCCURE_ERROR | E_STATE::E_STATE_ACTION_FAIL) ) {
    //     cmd->set_flag(E_FLAG::E_FLAG_STATE_ERROR, 1);
    // }
}


}   // valve_pkg
