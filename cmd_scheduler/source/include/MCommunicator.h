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
#include <CMDs/CuCMD.h>
#include <Common.h>

namespace comm {


class MCommunicator {
public:
    using CMDType = cmd::ICommand;
    using TListener = std::function<void(std::shared_ptr<CMDType>& /*command*/)>;
    using TProtoMapper = std::map<std::string /*pvd-id*/, std::string /*protocol file-path*/>;

private:
    using E_FLAG = common::E_FLAG;
    using CommHandler = std::shared_ptr<ICommunicator>;
    using TCommList = std::list<CommHandler>;
    using TCommMapper = std::map<std::string /*pvd-id*/, CommHandler /*communicator-instance*/>;
    using TListenMapper = std::map<std::string /*pvd-id*/, std::list<TListener> /*list of Listener-function*/>;
    using TPvdList = alias::IAliasSearcher::TPvdList;

public:
    MCommunicator( const std::string& app_path, std::string& file_path_alias, const TProtoMapper& mapper_pvd_proto );

    ~MCommunicator(void);

    void register_listener( const std::string& pvd_id, TListener func );

    void start( void );

    /* return value: msg-id if sending req-msg is failed, then msg-id == 0, vice verse msg-id != 0  */
    unsigned long request( std::string peer, std::string json_cmd, bool require_resp=true );

    // bool send_cmd_done(std::shared_ptr<CMDType> &cmd);

private:
    MCommunicator(void) = delete;
    MCommunicator(const MCommunicator&) = delete;
    MCommunicator& operator=(const MCommunicator&) = delete;

    void clear( void );

    void init( std::map<std::string, TPvdList>& pvd_mapper, const std::string& alias_file_path, 
                                                            const TProtoMapper& mapper_pvd_proto );

    std::shared_ptr<TCommList> get_comms( std::string& peer_app, std::string& peer_pvd, std::string proto_name );

    // void apply_sys_state(std::shared_ptr<CMDType> cmd);

    bool send( std::shared_ptr<CMDType> &cmd );

    void send_ack( std::string& pvd_id , std::shared_ptr<CMDType>& rcmd );

    bool send_without_payload( std::shared_ptr<alias::CAlias>& peer, E_FLAG flag, unsigned long msg_id=0);

    bool send_without_payload( const alias::CAlias& peer, E_FLAG flag, unsigned long msg_id, 
                               std::shared_ptr<ICommunicator>& comm);

    void call_listeners( std::string& pvd_id, std::shared_ptr<CMDType>& rcmd );

    /*****
     * Call-Back handler.
     */
    void cb_initialization(enum_c::ProviderType provider_type, bool flag_init, std::string pvd_id);

    void cb_connected(std::string peer_app, std::string peer_pvd, bool flag_connect, std::string pvd_id);

    void cb_receive_msg_handle(std::string peer_app, std::string peer_pvd, std::shared_ptr<payload::CPayload> payload, std::string pvd_id);

    void cb_abnormally_quit(const std::exception &e, std::string pvd_id);

    /****
     * Thread related functions
     */
    void create_threads(void);
    
    void destroy_threads(void);

    int run_keepalive(void);    // Keep-Alive react routin.

private:
    std::shared_ptr<alias::CAlias> _m_myself_;    // MCommunicator state

    std::shared_ptr<alias::IAliasSearcher> _m_alias_searcher_;

    TCommMapper _mm_comm_;          // multi-communicator

    TListenMapper _mm_listener_;    // multi-listener per provider-id.

    // Thread routine variables
    std::atomic<bool> _m_is_continue_;       // Thread continue-flag.

    std::thread _mt_keepaliver_; // Periodically, Thread that is charge of reacting Keep-Alive message.

    std::list<std::shared_ptr<alias::CAlias>> _ml_peers_;     // Connected Peer-list. (for keep-alive proc)

    std::mutex _mtx_peers_;

};


}   // namespace comm


#endif // _COMMUNICATOR_MANAGER_H_
