/***************************************************************************
 * 
 * C++ type: Concurrent multiple-Server with each specific Protocol. (IPv4)
 * 
 * *************************************************************************/

#include <ctime>
#include <iostream>
#include <unistd.h>

#include <IAppInf.h>
#include <CTranceiverCMD.h>
#include <sys_sigslot.h>

using namespace std::placeholders;

void slot_exit_program(int signal_num) {
    LOGW("Called. (sig-NUM = %d)", signal_num);
}

int main(int argc, char *argv[])
{
    std::cout << "CMD-scheduler Application." << std::endl;

    try {
        std::shared_ptr<cmd_pkg::CTranceiverCMD> tranceiver;

        // Create instance of System-signal receiver.
        auto signal_exit_program = sys_sigslot::CExitSig::get_instance();
        assert(signal_exit_program != NULL);

        // Create Communicator instance.
        auto handler = create_communicator("alias_udp_01", 
                                        "UDP_provider", 
                                        enum_c::ProviderType::E_PVDT_TRANS_UDP,
                                        atoi(argv[2]),
                                        argv[1],
                                        NULL,    // argv[3],
                                        argv[3]);

        std::cout << "Communicator-FW Version = " << handler->get_version() << std::endl;

        // Register Call-Back function pointer of CTranceiverCMD class.
        tranceiver = cmd_pkg::CTranceiverCMD::get_instance(&handler);
        handler->register_initialization_handler(std::bind(&cmd_pkg::CTranceiverCMD::cb_initialization, tranceiver.get(), _1, _2));
        handler->register_connection_handler(std::bind(&cmd_pkg::CTranceiverCMD::cb_connected, tranceiver.get(), _1, _2));
        handler->register_message_handler(std::bind(&cmd_pkg::CTranceiverCMD::cb_receive_msg_handle, tranceiver.get(), _1, _2));
        handler->register_unintended_quit_handler(std::bind(&cmd_pkg::CTranceiverCMD::cb_abnormally_quit, tranceiver.get(), _1));

        handler->init();

        signal_exit_program->connect(slot_exit_program);
        while( !signal_exit_program->get_signal() ) {
            // wait 1 seconds
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        tranceiver->exit();
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    std::cout << "Exit CMD-scheduler Application." << std::endl;
    return 0;
}
