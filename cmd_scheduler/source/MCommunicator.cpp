#include <cassert>
#include <iostream>

#include <stdio.h>
#include <unistd.h>

#include <logger.h>
#include <MCommunicator.h>
#include <IProtocolInf.h>
#include <CException.h>
#include <CMDs/CuCMD.h>

using namespace std::placeholders;

namespace comm {


/*********************************
 * Definition of Public Function.
 */
MCommunicator::MCommunicator( const std::string& app_path, std::string& file_path_alias, 
                                                           const TProtoMapper& mapper_pvd_proto ) {
    clear();
    try {
        _m_myself_ = std::make_shared<alias::CAlias>( app_path, "ALL-PVDs", true );
        if( _m_myself_.get() == NULL ) {
            throw std::runtime_error("Memory-Allication of _m_myself_ is failed.");
        }

        _m_time_synchor_ = std::make_shared<::cmd::CTimeSync>( _m_myself_, std::bind(&MCommunicator::keepalive,this,_1,_2,_3) );
        if( _m_time_synchor_.get() == NULL ) {
            throw std::runtime_error("Memory-Allication of _m_time_synchor_ is failed.");
        }

        // Get AliasSearcher
        _m_alias_searcher_ = alias::IAliasSearcher::get_instance( file_path_alias );
        if( _m_alias_searcher_.get() == NULL ) {
            throw std::runtime_error("Get Alias-Searcher instance is failed.");
        }

        auto pvd_mapper = _m_alias_searcher_->get_mypvds( app_path );
        init( pvd_mapper, file_path_alias, mapper_pvd_proto );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

MCommunicator::~MCommunicator(void) {
    clear();
}

void MCommunicator::register_listener( const std::string& pvd_id, TListener func ) {
    try {
        if( pvd_id.empty() == true || func == nullptr ) {
            std::string err = "pvd_id(" + pvd_id + ") is empty or Tlistener func is NULL.";
            throw std::invalid_argument(err);
        }

        _mm_listener_[pvd_id].push_back(func);
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void MCommunicator::start( void ) {
    try {
        for( auto itr=_mm_comm_.begin(); itr != _mm_comm_.end(); itr++ ) {
            LOGI("Start pvd-id(%s) instance.", itr->first.data());
            itr->second->init();
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/* return value: msg-id if sending req-msg is failed, then msg-id == 0, vice verse msg-id != 0  */
uint32_t MCommunicator::keepalive( const alias::CAlias& peer, const std::string& contents, common::StateType state ) {
    uint32_t msg_id = 0;

    try {
        cmd::ICommand::FlagType flag = E_FLAG::E_FLAG_NONE;
        CommHandler handler;
        std::string proto = cmd::CuCMD::PROTOCOL_NAME;
        std::string peer_app = peer.app_path;
        std::string peer_pvd = peer.pvd_id;
        std::shared_ptr<payload::CPayload> new_payload;

        // Search communicators that is connected with peer.
        std::shared_ptr<TCommList> comms_list = get_comms( peer_app, peer_pvd, proto );
        if( comms_list.get() == NULL ) {
            throw std::runtime_error("TCommList memory-allocation is failed.");
        }

        // Validation check.
        if( comms_list->size() <= 0 ) {
            std::string err = "Communicator for peer(" + peer_app + "/" + peer_pvd + ") of "+ proto +" Protocol is not exist.";
            throw std::logic_error(err);
        }

        if( comms_list->size() > 1 ) {
            std::string err = "Communicator for peer(" + peer_app + "/" + peer_pvd + ") of "+ proto +" Protocol is over than 1.";
            throw std::logic_error(err);
        }
        handler = *comms_list->begin();

        // Set flag & state variables
        flag |= E_FLAG::E_FLAG_KEEPALIVE;    // set KEEPALIVE message flag.
        state |= ::common::E_STATE::E_STATE_THR_KEEPALIVE;
        state |= _m_myself_->get_state(E_STATE::E_STATE_ALL);

        // Force-encode to packet.
        new_payload = cmd::CuCMD::force_encode( handler, contents, flag, state, msg_id );
        if( new_payload.get() == NULL ) {
            LOGERR("Encoding message is failed for peer(%s/%s) & proto(%s)", peer_app.data(), peer_pvd.data(), proto.data() );
            throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
        }

        // Send message.
        return handler->send(peer_app, peer_pvd, new_payload);
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        msg_id = 0;
    }
    
    return msg_id;
}

/* return value: msg-id if sending req-msg is failed, then msg-id == 0, vice verse msg-id != 0  */
uint32_t MCommunicator::request( const alias::CAlias& peer, const std::string& json_cmd, common::StateType state, bool require_resp ) {
    uint32_t msg_id = 0;

    try {
        cmd::ICommand::FlagType flag = E_FLAG::E_FLAG_NONE;
        CommHandler handler;
        std::string proto = cmd::CuCMD::PROTOCOL_NAME;
        std::string peer_app = peer.app_path;
        std::string peer_pvd = peer.pvd_id;
        std::shared_ptr<payload::CPayload> new_payload;

        // Search communicators that is connected with peer.
        std::shared_ptr<TCommList> comms_list = get_comms( peer_app, peer_pvd, proto );
        if( comms_list.get() == NULL ) {
            throw std::runtime_error("TCommList memory-allocation is failed.");
        }

        // Validation check.
        if( comms_list->size() <= 0 ) {
            std::string err = "Communicator for peer(" + peer_app + "/" + peer_pvd + ") of "+ proto +" Protocol is not exist.";
            throw std::logic_error(err);
        }

        if( comms_list->size() > 1 ) {
            std::string err = "Communicator for peer(" + peer_app + "/" + peer_pvd + ") of "+ proto +" Protocol is over than 1.";
            throw std::logic_error(err);
        }
        handler = *comms_list->begin();

        // Set flag & state variables
        flag |= E_FLAG::E_FLAG_REQUIRE_ACK;    // require ACK message.
        flag |= E_FLAG::E_FLAG_REQUIRE_ACT;    // require Action-Start message.
        if( require_resp == true ) {
            flag |= E_FLAG::E_FLAG_REQUIRE_RESP;    // require ACT-done message.
        }
        state |= _m_myself_->get_state(E_STATE::E_STATE_ALL);

        // Force-encode to packet.
        new_payload = cmd::CuCMD::force_encode( handler, json_cmd, flag, state, msg_id );
        if( new_payload.get() == NULL ) {
            LOGERR("Encoding message is failed for peer(%s/%s) & proto(%s)", peer_app.data(), peer_pvd.data(), proto.data() );
            throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
        }

        // Send message.
        return handler->send(peer_app, peer_pvd, new_payload);
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        msg_id = 0;
    }
    
    return msg_id;
}

// bool MCommunicator::send_cmd_done(std::shared_ptr<CMDType> &cmd) {
//     // TODO
//     return false;
// }



/*********************************
 * Definition of Private Function.
 */
void MCommunicator::clear( void ) {
    _m_myself_.reset();
    _m_time_synchor_.reset();
    _mm_keepalive_enabled_pvds_.clear();
    _m_alias_searcher_.reset();
    _mm_comm_.clear();
    _mm_listener_.clear();
}

void MCommunicator::init( std::map<std::string, TPvdList>& pvd_mapper, const std::string& alias_file_path, 
                                                                       const TProtoMapper& mapper_pvd_proto ) {
    try {
        for( auto itr=pvd_mapper.begin(); itr != pvd_mapper.end(); itr++ ) {
            for( auto itr_list=itr->second.begin(); itr_list != itr->second.end(); itr_list++ ) {
                auto pvd_id = (*itr_list)->name();
                auto app_path = (*itr_list)->path_parent();
                auto itr_proto = mapper_pvd_proto.find(pvd_id);

                try {
                    if( itr_proto == mapper_pvd_proto.end() ) {
                        std::string err = app_path + "/" + pvd_id + " is not exist in mapper_pvd_proto.";
                        throw std::invalid_argument(err);
                    }

                    auto handler = std::make_shared<ICommunicator>(app_path,
                                                                pvd_id,
                                                                alias_file_path,
                                                                itr_proto->second,
                                                                enum_c::ProviderMode::E_PVDM_BOTH);
                    if( handler.get() == NULL ) {
                        throw std::runtime_error("ICommunicator memory-allocation is failed.");
                    }

                    // For Keep-Alive enable, Add valid Provider-IDs.
                    init_keepalive( pvd_id, *handler->get_protocol_list(), cmd::CuCMD::PROTOCOL_NAME);

                    // Register Call-Back function pointer of MCommunicator class.
                    LOGD("Register pvd(%s) handler to _mm_comm_.", pvd_id.data());
                    _mm_comm_[pvd_id] = handler;
                    handler->register_initialization_handler( std::bind(&comm::MCommunicator::cb_initialization, this, _1, _2, pvd_id) );
                    handler->register_connection_handler( std::bind(&comm::MCommunicator::cb_connected, this, _1, _2, _3, pvd_id) );
                    handler->register_message_handler( std::bind(&comm::MCommunicator::cb_receive_msg_handle, this, _1, _2, _3, pvd_id) );
                    handler->register_unintended_quit_handler( std::bind(&comm::MCommunicator::cb_abnormally_quit, this, _1, pvd_id) );
                }
                catch ( const std::exception& e ) {
                    LOGERR("%s", e.what());
                    throw e;
                }
            }
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void MCommunicator::init_keepalive( std::string& pvd_id, std::list<std::string>& protocols, std::string target_protocol ) {
    try {
        if( target_protocol.empty() == true ) {
            throw std::invalid_argument("target_protocol is NULL. Please check it.");
        }

        // We only check First Protocol among all-of-them.
        auto itr=protocols.begin();
        if( itr == protocols.end() ) {
            LOGW("Protocol is Nothings with Provider %s", pvd_id.data());
            return ;
        }

        if( *itr == target_protocol ) {
            // register pvd_id to mapper.
            _mm_keepalive_enabled_pvds_[pvd_id] = target_protocol;
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

std::shared_ptr<MCommunicator::TCommList> MCommunicator::get_comms( std::string& peer_app, std::string& peer_pvd, std::string proto_name ) {
    std::shared_ptr<TCommList> comms_list;

    try {
        comms_list = std::make_shared<TCommList>();
        if( comms_list.get() == NULL ) {
            throw std::runtime_error("TCommList memory-allocation is failed.");
        }

        auto pvd_list = _m_alias_searcher_->get_mypvds_sendable( peer_app, peer_pvd );
        for( auto itr=pvd_list.begin(); itr != pvd_list.end(); itr++ ) {
            if( (*itr)->path_parent() != _m_myself_->app_path ) {
                continue;
            }

            auto itr_handler = _mm_comm_.find( (*itr)->name() );
            if( itr_handler != _mm_comm_.end() ) {
                auto proto_list = itr_handler->second->get_protocol_list();
                for( auto itr_proto=proto_list->begin(); itr_proto != proto_list->end(); itr_proto++ ) {
                    if( (*itr_proto) == proto_name ) {
                        LOGI("Found %s Protocol in PVD(%s).", proto_name.data(), itr_handler->first.data() );
                        comms_list->push_back(itr_handler->second);
                    }
                }
            }
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return comms_list;
}

void MCommunicator::apply_sys_state(std::shared_ptr<::cmd::CuCMD>& cmd) {
    try {
        if( cmd.get() == NULL ) {
            throw std::invalid_argument("CMD is NULL.");
        }

        cmd->set_state( _m_myself_->get_state(::common::E_STATE::E_STATE_ALL) );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

bool MCommunicator::send( std::shared_ptr<CMDType> &cmd ) {
    try {
        if( cmd.get() == NULL ) {
            throw std::invalid_argument("Invalid CMD is NULL.");
        }
        
        CommHandler handler;
        std::string proto = cmd->proto_name();
        std::string peer_app = cmd->who().get_app();
        std::string peer_pvd = cmd->who().get_pvd();
        std::shared_ptr<payload::CPayload> new_payload;

        // Search communicators that is connected with peer.
        std::shared_ptr<TCommList> comms_list = get_comms( peer_app, peer_pvd, proto );
        if( comms_list.get() == NULL ) {
            throw std::runtime_error("TCommList memory-allocation is failed.");
        }

        // Validation check.
        if( comms_list->size() <= 0 ) {
            std::string err = "Communicator for peer(" + peer_app + "/" + peer_pvd + ") of "+ proto +" Protocol is not exist.";
            throw std::logic_error(err);
        }

        if( comms_list->size() > 1 ) {
            std::string err = "Communicator for peer(" + peer_app + "/" + peer_pvd + ") of "+ proto +" Protocol is over than 1.";
            throw std::logic_error(err);
        }
        handler = *comms_list->begin();

        // Encode cmd to packet.
        new_payload = cmd->encode( handler );
        if( new_payload.get() == NULL ) {
            LOGERR("Encoding message is failed for peer(%s/%s) & proto(%s)", peer_app.data(), peer_pvd.data(), proto.data() );
            throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
        }

        // Send message.
        return handler->send(peer_app, peer_pvd, new_payload);
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
    }
    
    return false;
}

void MCommunicator::send_ack( std::string& pvd_id , std::shared_ptr<CMDType>& rcmd ) {
    try {
        if( (bool)(rcmd->get_flag(E_FLAG::E_FLAG_REQUIRE_ACK)) == false ) {
            LOGW("Don't send ACK message on pvd(%s).", pvd_id.data());
            return;
        }

        auto itr = _mm_comm_.find( pvd_id );
        if( itr == _mm_comm_.end() ) {
            std::string err = "Can not find pvd-instance. (name=" + pvd_id + ")";
            throw std::logic_error(err);
        }

        // If rcmd require ACK, then send ACK message.
        LOGD("Try Sending ACK msg of Msg-ID(%d).", rcmd->get_id());
        send_without_payload( rcmd->get_from(), E_FLAG::E_FLAG_ACK_MSG, rcmd->get_id(), itr->second );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
    }
}

bool MCommunicator::send_without_payload( std::shared_ptr<alias::CAlias>& peer, E_FLAG flag, unsigned long msg_id) {
    bool result = true;
    try {
        std::shared_ptr<TCommList> comms_list = get_comms( peer->app_path, peer->pvd_id, cmd::CuCMD::PROTOCOL_NAME );
        if( comms_list.get() == NULL ) {
            throw std::runtime_error("TCommList memory-allocation is failed.");
        }

        if( comms_list->size() <= 0 ) {
            LOGW("Communicator of %s Protocol is not exist.", cmd::CuCMD::PROTOCOL_NAME );
        }

        for( auto itr = comms_list->begin(); itr != comms_list->end(); itr++ ) {
            if( send_without_payload( *peer, flag, msg_id, *itr) == false ) {
                LOGERR("send_without_payload is failed. (peer: %s/%s:%u)", peer->app_path.data(), peer->pvd_id.data(), msg_id);
                result = false;
            }
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        result = false;
    }

    return result;
}

bool MCommunicator::send_without_payload( const alias::CAlias& peer, 
                                          E_FLAG flag, 
                                          unsigned long msg_id, 
                                          std::shared_ptr<ICommunicator>& comm) {

    std::shared_ptr<cmd::CuCMD> simple_cmd;
    std::shared_ptr<payload::CPayload> new_payload;
    assert( flag == E_FLAG::E_FLAG_ACK_MSG || 
            flag == E_FLAG::E_FLAG_ACTION_START );

    try {
        if( comm.get() == NULL ) {
            throw std::invalid_argument("Comm is NULL.");
        }

        if( msg_id == 0 ) {
            throw std::logic_error("We need specific msg-id. It's NULL.");
        }
        
        LOGD("Is empty of MySelf? (%u)", _m_myself_->empty());
        simple_cmd = std::make_shared<cmd::CuCMD>(*_m_myself_, flag);
        // Check current state & set it to cmd.
        apply_sys_state(simple_cmd);
        // Set message-ID.
        simple_cmd->set_id(msg_id);

        // Encode cmd to packet.
        new_payload = simple_cmd->encode( comm );
        if( new_payload.get() == NULL ) {
            LOGERR("Encoding message is failed. Please check it.");
            throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
        }

        // Send message.
        return comm->send(peer.app_path, peer.pvd_id, new_payload);
    }
    catch ( const CException& e ) {
        LOGERR("%s", e.what());
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

void MCommunicator::call_listeners( std::string& pvd_id, std::shared_ptr<CMDType>& rcmd ) {
    try {
        auto itr = _mm_listener_.find(pvd_id);
        if( itr == _mm_listener_.end() ) {
            LOGW("Nothing is exist in Listener-mapper for pvd_id(%s).", pvd_id.data());
            return ;
        }

        for( auto itr_list=itr->second.begin(); itr_list != itr->second.end(); itr_list++ ) {
            (*itr_list)(rcmd);
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

/*****
 * Call-Back handler.
 */
void MCommunicator::cb_initialization(enum_c::ProviderType provider_type, bool flag_init, std::string pvd_id) {
    if( flag_init == true ) {
        LOGI("Init-done pvd_id=%s [pvd-type=%u]", pvd_id.data(), provider_type);
    }
    else {
        LOGI("Quit-done pvd_id=%s [pvd-type=%u]", pvd_id.data(), provider_type);
    }
}

void MCommunicator::cb_connected(std::string peer_app, std::string peer_pvd, bool flag_connect, std::string pvd_id) {
    try {
        if( _mm_keepalive_enabled_pvds_.find(pvd_id) == _mm_keepalive_enabled_pvds_.end() ) {
            LOGW("\"%s\" is provider-ID that KEEPALIVE function is disabled.", pvd_id.data());
            return ;
        }

        if( flag_connect == true ) {
            LOGI("Connected Peer. (app-path=%s, pvd-id=%s)", peer_app.data(), peer_pvd.data());
            _m_time_synchor_->append_peer( peer_app, peer_pvd );
        }
        else {
            LOGI("Disconnected Peer. (app-path=%s, pvd-id=%s)", peer_app.data(), peer_pvd.data());
            _m_time_synchor_->remove_peer( peer_app, peer_pvd );
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void MCommunicator::cb_receive_msg_handle(std::string peer_app, std::string peer_pvd, 
                                          std::shared_ptr<payload::CPayload> payload, std::string pvd_id) {
    try {
        std::string proto_name = payload->get_name();

        std::cout << "************************************" << std::endl;
        std::cout << "* 1. Peer-ID : " << peer_app << "/" << peer_pvd << std::endl;
        std::cout << "* 2. CPayload-Name : " << proto_name << std::endl;
        std::cout << "************************************" << std::endl;

        std::shared_ptr<CMDType> rcmd;
        std::shared_ptr<IProtocolInf> protocol = payload->get(proto_name);

        if( proto_name == CMDType::PROTOCOL_NAME ) {
            rcmd = std::make_shared<CMDType>( peer_app, peer_pvd );
        }
        else if ( proto_name == cmd::CuCMD::PROTOCOL_NAME ) {
            rcmd = std::make_shared<cmd::CuCMD>( peer_app, peer_pvd );
        }
        
        // Parsing of received-CMD.
        if( rcmd->decode( protocol ) == false ) {
            std::string err = "Decoding message is failed. (peer=" + peer_app + "/" + peer_pvd + ", proto=" + proto_name + ")";
            throw std::logic_error(err);
        }

        // Processing received KEEPALIVE msg.
        if ( proto_name == cmd::CuCMD::PROTOCOL_NAME ) {
            if( rcmd->get_flag(E_FLAG::E_FLAG_KEEPALIVE) != 0 ) {
                LOGI("Arrive KeepAlive-message from peer(%s/%s)", peer_app.data(), peer_pvd.data());
                _m_time_synchor_->update_keepalive( std::dynamic_pointer_cast<cmd::CuCMD>(rcmd) );
                return ;
            }
        }

        // Processing received CMD msg. (Publishing CMD-event to APPs-Listener.)
        if( _m_myself_->get_state(E_STATE::E_STATE_OUT_OF_SERVICE) != 0 ) {
            send_ack( pvd_id , rcmd );  // Send ACK message to peer with OUT_OF_SERVICE State.
            throw std::out_of_range("All-Service are out-of-service state.");
        }

        call_listeners(pvd_id, rcmd);
        send_ack( pvd_id , rcmd );  // Send ACK message to peer.
    }
    catch ( const std::out_of_range& e ) {
        LOGW("%s", e.what());
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
    }
}

void MCommunicator::cb_abnormally_quit(const std::exception &e, std::string pvd_id) {
    LOGERR("pvd-id=%s: %s", pvd_id.data(), e.what());
}



}   // namespace comm
