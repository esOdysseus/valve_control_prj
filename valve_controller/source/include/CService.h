#ifndef _H_SERVICE_CLASS_MANAGER_H_
#define _H_SERVICE_CLASS_MANAGER_H_

#include <memory>
#include <string>

#include <CuCMD/MCommunicator.h>
#include <CController.h>

namespace service {


class CService {
public:
    static const std::string APP_PATH;
    static const std::string PVD_COMMANDER;
    static const std::string PEER_CMD_SCHEDULER;
    static const std::string PEER_PROVIDER;

public:
    static std::shared_ptr<CService> get_instance( void );

    bool init( std::string file_path_alias, std::string file_path_proto );

    void start( void );

    void exit( void );

    ~CService( void );

private:
    CService(const CService&) = delete;             // copy constructor
    CService& operator=(const CService&) = delete;  // copy operator
    CService(CService&&) = delete;                  // move constructor
    CService& operator=(CService&&) = delete;       // move operator

    CService( void );

    void clear( void );

    void cb_changed_svc_state( bool service_on );

private:
    std::shared_ptr<comm::MCommunicator>  _m_comm_mng_;

    valve_pkg::CController _m_ctrller_;   // Valve-Controller class.

};


}   // service


#endif // _H_SERVICE_CLASS_MANAGER_H_