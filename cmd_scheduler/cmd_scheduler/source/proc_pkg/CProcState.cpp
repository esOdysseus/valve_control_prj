#include <time.h>
#include <cassert>

#include <logger.h>
#include <CProcState.h>
#include <CException.h>

namespace proc_pkg {

/***********************************
 * Definition of Public Function
 */
CProcState::CProcState(void): IProc(SELF_NAME) {

}

CProcState::~CProcState(void) {

}

std::shared_ptr<CProcState::PacketType> CProcState::make_packet_for_dbgcmd(std::shared_ptr<CMDDebug> &cmd) {
    std::shared_ptr<PacketType> packet;

    return packet;
}

void CProcState::register_sent_msg(uint32_t msg_id, bool result) {

}

bool CProcState::append_time_for_updating(double time_in_packet, double time_on_receiv) {
    LOGD("Called.");

    // TODO
    return true;
}

/***********************************
 * Definition of Protected Function
 */
bool CProcState::data_update(TaskType type, std::shared_ptr<CMDType> cmd) {
    LOGD("Called.");
    struct timespec time;
    
    try {
        switch(type) {
        case TaskType::E_PROC_TASK_DEFAULT:
            LOGD("Task : Default. (Empty-Processing)");
            break;
        case TaskType::E_PROC_TASK_TIME_UPDATE:
            LOGW("Task : try to update local Sys-Time.");

            time = cmd->get_when<struct timespec>();
            // set local system-time. (unit seconds)
            if ( stime(&time.tv_sec) == -1 ) {
                LOGERR("Setting of 'local Sys-Time' is failed.");
                LOGERR("%d: %s", errno, strerror(errno));
                if( errno == 1 ) {
                    LOGW("You have to run cmd_scheduler with 'root' permission.");
                }
                throw CException(E_ERROR::E_ERR_FAIL_TIME_UPDATE);
            }
            break;
        default:
            LOGERR("Not Supported Task-type(%d)", type);
            throw CException(E_ERROR::E_ERR_NOT_SUPPORTED_TYPE);
        }
        return true;
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}



}   // namespace proc_pkg
