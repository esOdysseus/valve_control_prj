#ifndef _COMMUNICATOR_MANAGER_H_
#define _COMMUNICATOR_MANAGER_H_

#include <map>
#include <list>
#include <string>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

#include <ICommunicator.h>
#include <IAliasSearcher.h>
#include <ICommand.h>
#include <CuCMD/CuCMD.h>
#include <Common.h>
#include <CuCMD/CTimeSync.h>

namespace comm {


class MCommunicator {
public:
    using CMDType = cmd::ICommand;
    using TLsvcState = ::cmd::CTimeSync::TFsvcState;
    using TListener = std::function<void(std::shared_ptr<CMDType>& /*command*/)>;
    using TProtoMapper = std::map<std::string /*pvd-id*/, std::string /*protocol file-path*/>;

private:
    using E_FLAG = common::E_FLAG;
    using E_STATE = common::E_STATE;
    using CommHandler = std::shared_ptr<ICommunicator>;
    using TCommList = std::list<CommHandler>;
    using TCommMapper = std::map<std::string /*pvd-id*/, CommHandler /*communicator-instance*/>;
    using TListenMapper = std::map<std::string /*pvd-id*/, std::list<TListener> /*list of Listener-function*/>;
    using TPvdList = alias::IAliasSearcher::TPvdList;

public:
    MCommunicator( const std::string& app_path, 
                   std::string& file_path_alias, 
                   const TProtoMapper& mapper_pvd_proto, 
                   const double max_holding_time = MAX_HOLD_TIME );

    ~MCommunicator(void);

    void register_listener( const std::string& pvd_id, TListener func );

    void register_listener_svc_state( TLsvcState func );

    std::shared_ptr<alias::CAlias> get_myself(void);

    void start( void );

    /** for client mode. */
    bool connect_auto( const std::string& peer_app, const std::string& peer_pvd, const std::string& pvd_id );

    /** for client mode. */
    bool disconnect( const std::string& peer_app, const std::string& peer_pvd );

    /* return value: msg-id if sending req-msg is failed, then msg-id == 0, vice verse msg-id != 0  */
    uint32_t keepalive( const alias::CAlias& peer, const std::string& data, common::StateType state );

    /* return value: msg-id if sending req-msg is failed, then msg-id == 0, vice verse msg-id != 0  */
    uint32_t request( const alias::CAlias& peer, const std::string& json_cmd, common::StateType state, bool require_resp=true );

    bool notify_action_start( const alias::CAlias& peer, unsigned long msg_id, E_STATE state );// for client mode.

    bool notify_action_done( const alias::CAlias& peer, unsigned long msg_id, E_STATE state ); // for client mode.

private:
    MCommunicator(void) = delete;
    MCommunicator(const MCommunicator&) = delete;
    MCommunicator& operator=(const MCommunicator&) = delete;

    void clear( void );

    void init( std::map<std::string, TPvdList>& pvd_mapper, const std::string& alias_file_path, 
                                                            const TProtoMapper& mapper_pvd_proto );

    void init_keepalive( std::string& pvd_id, std::list<std::string>& protocols, std::string target_protocol );

    std::shared_ptr<TCommList> get_comms( const std::string& peer_app, const std::string& peer_pvd, std::string proto_name );

    void apply_sys_state(std::shared_ptr<::cmd::CuCMD>& cmd, common::StateType state=E_STATE::E_NO_STATE);

    bool send( std::shared_ptr<CMDType> &cmd );

    void send_ack( std::string& pvd_id , std::shared_ptr<CMDType>& rcmd );

    bool send_without_payload( const alias::CAlias& peer, E_FLAG flag, unsigned long msg_id=0, E_STATE state=E_STATE::E_NO_STATE);

    bool send_without_payload( const alias::CAlias& peer, E_FLAG flag, unsigned long msg_id, 
                               std::shared_ptr<ICommunicator>& comm, E_STATE state=E_STATE::E_NO_STATE);

    void call_listeners( std::string& pvd_id, std::shared_ptr<CMDType>& rcmd );

    /*****
     * Call-Back handler.
     */
    void cb_initialization(enum_c::ProviderType provider_type, bool flag_init, std::string pvd_id);

    void cb_connected(std::string peer_app, std::string peer_pvd, bool flag_connect, std::string pvd_id);

    void cb_receive_msg_handle(std::string peer_app, std::string peer_pvd, std::shared_ptr<payload::CPayload> payload, std::string pvd_id);

    void cb_abnormally_quit(const std::exception &e, std::string pvd_id);

private:
    std::shared_ptr<alias::CAlias> _m_myself_;    // MCommunicator state

    std::shared_ptr<::cmd::CTimeSync> _m_time_synchor_;

    std::map<std::string/*pvd_id*/, std::string/*protocol-name*/> _mm_keepalive_enabled_pvds_;

    std::shared_ptr<alias::IAliasSearcher> _m_alias_searcher_;

    TCommMapper _mm_comm_;          // multi-communicator

    TListenMapper _mm_listener_;    // multi-listener per provider-id.

    static constexpr const double MAX_HOLD_TIME = 24 * 3600.0;     // 24 hour

};


}   // namespace comm


#endif // _COMMUNICATOR_MANAGER_H_
