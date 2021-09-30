/***************************************************************************
 * 
 * C++ type: Concurrent multiple-Server with each specific Protocol. (IPv4)
 * 
 * *************************************************************************/

#include <cassert>
#include <ctime>
#include <iostream>
#include <unistd.h>

#include <CService.h>
#include <sys_sigslot.h>
#include <version.h>
#include <logger.h>

using namespace std::placeholders;

void slot_exit_program(int signal_num) {
    LOGW("Called. (sig-NUM = %d)", signal_num);
}

int main(int argc, char *argv[])
{
    std::cout << "Valve-control Application. (Version: " << STRING_OF_APP_VERSION << ")" << std::endl;

    if( argc != 3 ) {
        std::cout << "===========================================" << std::endl;
        std::cout << "= Please insert following arguments." << std::endl;
        std::cout << "=  - arg-01: path of json-file for Aliases." << std::endl;
        std::cout << "=  - arg-02: path of json-file for Protocol." << std::endl;
        std::cout << std::endl;
        return -1;
    }

    try {
        // Create instance of System-signal receiver.
        auto signal_exit_program = sys_sigslot::CExitSig::get_instance();
        assert(signal_exit_program != NULL);


        // Create service.
        auto service = service::CService::get_instance();
        service->init( argv[1], argv[2] );
        // Start service.
        service->start();


        // Wait signal
        signal_exit_program->connect(slot_exit_program);
        while( !signal_exit_program->get_signal() ) {
            // wait 1 seconds
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Exit service.
        service->exit();
    }
    catch( const std::exception &e) {
        LOGERR("%s", e.what());
    }

    std::cout << "Exit Valve-control Application." << std::endl;
    return 0;
}
