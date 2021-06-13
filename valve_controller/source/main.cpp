/***************************************************************************
 * 
 * C++ type: Concurrent multiple-Server with each specific Protocol. (IPv4)
 * 
 * *************************************************************************/

#include <cassert>
#include <ctime>
#include <iostream>
#include <unistd.h>

#include <logger.h>
#include <ICommunicator.h>
#include <CCommunicator.h>
#include <sys_sigslot.h>
#include <version.h>

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
        std::string app_path = "Valve-Controller";
        std::string pvd_id = "cmd_receiver";
        // Create instance of System-signal receiver.
        auto signal_exit_program = sys_sigslot::CExitSig::get_instance();
        assert(signal_exit_program != NULL);

        // Create Communicator instance.
        auto handler = std::make_shared<ICommunicator>( app_path,
                                                        pvd_id,
                                                        argv[1],
                                                        argv[2],
                                                        enum_c::ProviderMode::E_PVDM_BOTH);
        if( handler.get() == NULL ) {
            throw std::runtime_error("Can not create ICommunicator handler.");
        }
        
        std::cout << "Communicator-FW Version = " << handler->get_version() << std::endl;

        // Register Call-Back function pointer of CCommunicator class.
        valve_pkg::CCommunicator vController( handler, app_path, pvd_id );
        handler->register_initialization_handler(std::bind(&valve_pkg::CCommunicator::cb_initialization, &vController, _1, _2));
        handler->register_connection_handler(std::bind(&valve_pkg::CCommunicator::cb_connected, &vController, _1, _2, _3));
        handler->register_message_handler(std::bind(&valve_pkg::CCommunicator::cb_receive_msg_handle, &vController, _1, _2, _3));
        handler->register_unintended_quit_handler(std::bind(&valve_pkg::CCommunicator::cb_abnormally_quit, &vController, _1));

        handler->init();

        signal_exit_program->connect(slot_exit_program);
        while( !signal_exit_program->get_signal() ) {
            // wait 1 seconds
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch( const std::exception &e) {
        LOGERR("%s", e.what());
    }

    std::cout << "Exit Valve-control Application." << std::endl;
    return 0;
}
