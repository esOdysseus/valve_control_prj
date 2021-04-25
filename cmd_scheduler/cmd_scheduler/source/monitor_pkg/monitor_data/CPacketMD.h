#ifndef _C_PACKET_MONITORING_DATA_H_
#define _C_PACKET_MONITORING_DATA_H_

#include <memory>

#include <IMD.h>
#include <MData.h>
#include <IProc.h>

namespace monitor_pkg {

using E_MNT_Packet_FLAG = enum class E_MNT_Packet_FLAG {
    EMNT_FLAG_NONE = 0,
    EMNT_FLAG_RETRANS_CNT  = 0x00000003,   // 2 bit
    EMNT_FLAG_SENT_OK      = 0x00000004,   // 1 bit
    EMNT_FLAG_ACK_RCV      = 0x00000008,   // 1 bit
    EMNT_FLAG_ACT_DONE_RCV = 0x00000010,   // 1 bit
    EMNT_FLAG_RESP_RCV     = 0x00000020,   // 1 bit
    EMNT_FLAG_ACK_REQUIRE  = 0x00000040,   // 1 bit
    EMNT_FLAG_ACT_REQUIRE  = 0x00000080,   // 1 bit
    EMNT_FLAG_RESP_REQUIRE = 0x00000100,   // 1 bit
    EMNT_FLAG_ERR_OCCURE   = 0x00000200    // 1 bit
};

using E_MNT_Packet_ERROR = enum class E_MNT_Packet_ERROR {
    EMNT_ERROR_NONE = 0,
    EMNT_ERROR_STATE        = 0x0000FFFF    // 16 bit
};

/*************************************
 * Declation of CPacketMD class
 */
class CPacketMD: public IMD<uint32_t,                       // _ID_TYPE_
                            uint32_t,                       // _FLAG_TYPE_
                            uint32_t,                       // _ERR_TYPE_
                            E_MNT_Packet_FLAG,              // _ENUM_MNT_FLAG_
                            E_MNT_Packet_ERROR> {           // _ENUM_MNT_ERR_
public:
    using DataType = proc_pkg::IProc::PacketType;

public:
    CPacketMD(IDType msg_id, std::shared_ptr<DataType> &packet);

    ~CPacketMD(void);

    std::shared_ptr<DataType> get_packet(void);

private:
    bool set_flag_extra(E_MNT_FLAG pos, FlagType value);

private:
    std::shared_ptr<DataType> _packet_;

};


}

#endif // _C_PACKET_MONITORING_DATA_H_