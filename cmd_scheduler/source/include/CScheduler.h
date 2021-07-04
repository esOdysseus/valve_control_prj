#ifndef _CLASS_SCHEDULER_SERVICE_H_
#define _CLASS_SCHEDULER_SERVICE_H_

#include <memory>
#include <string>
#include <MCommunicator.h>
#include <ICommand.h>
// #include <CMDs/CuCMD.h>

namespace service {


class CScheduler {
public:
    static const std::string APP_PATH;
    static const std::string PVD_COMMANDER;
    static const std::string PVD_DEBUGGER;

public:
    static std::shared_ptr<CScheduler> get_instance( void );

    bool init( std::string file_path_alias, std::string file_path_proto );

    void start( void );

    void exit( void );

    ~CScheduler( void );

private:
    CScheduler(const CScheduler&) = delete;             // copy constructor
    CScheduler& operator=(const CScheduler&) = delete;  // copy operator
    CScheduler(CScheduler&&) = delete;                  // move constructor
    CScheduler& operator=(CScheduler&&) = delete;       // move operator

    CScheduler( void ) = default;

    void clear( void );

    void handle_command( std::shared_ptr<cmd::ICommand>& cmd );

    // void handle_command( std::shared_ptr<cmd::CuCMD>& cmd );

    // void handle_def_command( std::shared_ptr<cmd::ICommand>& cmd );

private:
    std::shared_ptr<comm::MCommunicator>  _m_comm_mng_;

};


}   // namespace service


#endif // _CLASS_SCHEDULER_SERVICE_H_