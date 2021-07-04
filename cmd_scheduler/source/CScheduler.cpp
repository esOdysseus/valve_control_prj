#include <CScheduler.h>

#include <logger.h>

using namespace std::placeholders;

namespace service {


const std::string CScheduler::APP_PATH = "CMD-Scheduler";
const std::string CScheduler::PVD_COMMANDER = "cmd_tranceiver";
const std::string CScheduler::PVD_DEBUGGER = "def_debugger";


/*********************************
 * Definition of Public Function.
 */
std::shared_ptr<CScheduler> CScheduler::get_instance( void ) {
    static std::shared_ptr<CScheduler> _instance_( new CScheduler() );
    return _instance_;
}

bool CScheduler::init( std::string file_path_alias, std::string file_path_proto ) {
    try {
        const comm::MCommunicator::TProtoMapper mapper = {
            { PVD_COMMANDER, file_path_proto },
            { PVD_DEBUGGER, std::string() }
        };
        
        _m_comm_mng_ = std::make_shared<comm::MCommunicator>( APP_PATH, file_path_alias, mapper );
        if( _m_comm_mng_.get() == NULL ) {
            throw std::runtime_error("MCommunicator mem-allocation is failed.");
        }

        _m_comm_mng_->register_listener( PVD_COMMANDER, std::bind(&CScheduler::handle_command, this, _1) );
        _m_comm_mng_->register_listener( PVD_DEBUGGER, std::bind(&CScheduler::handle_command, this, _1) );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CScheduler::start( void ) {
    if( _m_comm_mng_.get() == NULL ) {
        throw std::runtime_error("MCommunicator is NULL.");
    }

    _m_comm_mng_->start();
}

void CScheduler::exit( void ) {
    clear();
}

CScheduler::~CScheduler(void) {
    exit();
    LOGD("Terminated.");
}


/*********************************
 * Definition of Private Function.
 */
void CScheduler::clear( void ) {
    _m_comm_mng_.reset();
}

void CScheduler::handle_command( std::shared_ptr<cmd::ICommand>& cmd ) {
    try {
        LOGD("Enter");
        if( cmd.get() == NULL ) {
            throw std::invalid_argument("Invalid CMD is NULL.");
        }

        ;   // TODO
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

// void CScheduler::handle_command( std::shared_ptr<cmd::CuCMD>& cmd ) {
//     try {
//         if( cmd.get() == NULL ) {
//             throw std::invalid_argument("Invalid CMD is NULL.");
//         }

//         ;   // TODO
//     }
//     catch ( const std::exception& e ) {
//         LOGERR("%s", e.what());
//         throw e;
//     }
// }

// void CScheduler::handle_def_command( std::shared_ptr<cmd::ICommand>& cmd ) {
//     try {
//         if( cmd.get() == NULL ) {
//             throw std::invalid_argument("Invalid CMD is NULL.");
//         }

//         ;   // TODO
//     }
//     catch ( const std::exception& e ) {
//         LOGERR("%s", e.what());
//         throw e;
//     }
// }



}   // service