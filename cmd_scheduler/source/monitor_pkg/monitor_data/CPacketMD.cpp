#include <logger.h>
#include <CPacketMD.h>

namespace monitor_pkg {

using namespace std::placeholders;

/*********************************
 * Definition of Public Function.
 */
CPacketMD::CPacketMD(IDType msg_id, std::shared_ptr<DataType> &packet )
: IMD(msg_id, std::bind(&CPacketMD::set_flag_extra, this, _1, _2)) {
    if ( packet.get() == NULL ) {
        throw CException(E_ERROR::E_ERR_INVALID_NULL_VALUE);
    }
    _packet_ = packet;
}

CPacketMD::~CPacketMD(void) {
    _id_ = 0;
    _packet_.reset();
}

std::shared_ptr<CPacketMD::DataType> CPacketMD::get_packet(void) {
    return _packet_;
}

/**********************************
 * Definition of Private Function.
 */
bool CPacketMD::set_flag_extra(E_MNT_FLAG pos, FlagType value) {
    LOGD("Called.");
    
    if ( pos == E_MNT_FLAG::EMNT_FLAG_SENT_OK ) {
        set_time();
    }
    return true;
}

}