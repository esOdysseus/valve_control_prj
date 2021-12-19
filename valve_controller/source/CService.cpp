#include <unistd.h>

#include <CService.h>
#include <logger.h>

using namespace std::placeholders;

namespace service {


const std::string CService::APP_PATH = "Valve-Controller";
const std::string CService::PVD_COMMANDER = "cmd_receiver";
const std::string CService::PEER_CMD_SCHEDULER = "CMD-Scheduler";
const std::string CService::PEER_PROVIDER = "cmd_transceiver";


/*********************************
 * Definition of Public Function.
 */
std::shared_ptr<CService> CService::get_instance( void ) {
    static std::shared_ptr<CService> _instance_( new CService() );
    return _instance_;
}

bool CService::init( std::string file_path_alias, std::string file_path_proto ) {
    try {
        const comm::MCommunicator::TProtoMapper mapper = {
            { PVD_COMMANDER, file_path_proto }
        };
        
        _m_comm_mng_ = std::make_shared<comm::MCommunicator>( APP_PATH, file_path_alias, mapper, 24 * 3600.0 );
        if( _m_comm_mng_.get() == NULL ) {
            throw std::runtime_error("MCommunicator mem-allocation is failed.");
        }

        _m_ctrller_.init(_m_comm_mng_);
        _m_comm_mng_->register_listener( PVD_COMMANDER, std::bind(&valve_pkg::CController::receive_command, &_m_ctrller_, _1) );
        _m_comm_mng_->register_listener_svc_state( std::bind(&CService::cb_changed_svc_state, this, _1) );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CService::start( void ) {
    try {
        if( _m_comm_mng_.get() == NULL ) {
            throw std::runtime_error("MCommunicator is NULL.");
        }

        _m_comm_mng_->start();
        sleep(1);
        if( _m_comm_mng_->connect_auto( PEER_CMD_SCHEDULER, PEER_PROVIDER, PVD_COMMANDER ) == false ) {
            throw std::runtime_error("Can not start Connect-thread for peer(CMD-SCHEDULER).");
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CService::exit( void ) {
    LOGW("Enter.");
    if( _m_comm_mng_.get() != NULL ) {
        _m_comm_mng_->disconnect( PEER_CMD_SCHEDULER, PEER_PROVIDER );
        _m_comm_mng_.reset();
    }
    _m_ctrller_.soft_exit();
    clear();
}

CService::~CService(void) {
    exit();
    LOGD("Terminated.");
}


/*********************************
 * Definition of Private Function.
 */
CService::CService( void ) {
    clear();
}

void CService::clear( void ) {
    _m_comm_mng_.reset();
}

void CService::cb_changed_svc_state( bool service_on ) {
    try {
        _m_ctrller_.set_service_indicator( service_on );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}


}   // service
