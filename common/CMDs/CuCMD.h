#ifndef _UNIVERSAL_COMMAND_H_
#define _UNIVERSAL_COMMAND_H_

#include <ctime>
#include <string>
#include <memory>

#include <ICommand.h>
#include <Common.h>


/*******************************
 * Command(CMD)-format
 ******************************/
typedef enum E_FLAG {
    E_FLAG_NONE = 0,
    E_FLAG_REQUIRE_RESP = 0x01,     // 0: not-require RESP      , 1: require RESP
    E_FLAG_REQUIRE_ACK  = 0x02,     // 0: not-require ACK       , 1: require ACK
    E_FLAG_REQUIRE_ACT  = 0x04,     // 0: not-require ACT-DONE  , 1: require ACT-DONE
    E_FLAG_KEEPALIVE    = 0x08,     // 0: not keep-alive msg    , 1: keep-alive msg
    E_FLAG_RESP_MSG     = 0x10,     // 0: REQ/PUB message       , 1: RESP message
    E_FLAG_ACK_MSG      = 0x20,     // 0: not ack message       , 1: ACK message
    E_FLAG_ACTION_DONE  = 0x40,     // 0: not act-done msg      , 1: ACT-DONE msg
    E_FLAG_STATE_ERROR  = 0x80,     // 0: normal state          , 1: abnormal state
    E_FLAG_ALL          = 0xFF
} E_FLAG;


/*******************************
 * Definition of Class.
 */
namespace cmd {


class CuCMD: public ICommand {
public:
    using FlagType = uint8_t;
    static constexpr const char* NAME = "uCMD";
    static constexpr const char* PROTOCOL_NAME = "CPUniversalCMD";

public:
    CuCMD(std::string my_app_path, std::string my_pvd_id);

    CuCMD( alias::CAlias& myself, FlagType flag_val);

    CuCMD( const CuCMD& cmd );

    ~CuCMD(void);

    std::string name(void) override { return NAME; }

    std::string proto_name(void) override { return PROTOCOL_NAME; }

    // presentator
    bool decode(std::shared_ptr<IProtocolInf>& protocol) override;

    std::shared_ptr<payload::CPayload> encode( std::shared_ptr<ICommunicator>& handler ) override;

    // getter
    unsigned long get_id(void) { return _msg_id_; }

    FlagType get_flag(FlagType pos=E_FLAG::E_FLAG_ALL);

    uint16_t get_state(void) { return _state_; }

    double get_send_time(void) { return _send_time_d_; }

    // setter
    void set_id(unsigned long value);

    void set_flag(E_FLAG pos, FlagType value=0);

    void set_state(uint16_t value);

    // printer
    std::string print_send_time(void);  // print when-data for human-readable.

private:
    void clear(void);

    uint32_t gen_random_msg_id(void);

private:
    // Data-Structure for Decoded packet.
    uint8_t _flag_;
    
    uint16_t _state_;

    uint32_t _msg_id_;

    // send/receive time
    double _send_time_d_;       // UTC time with nano-seconds.

};


}   // namespace cmd


#endif // _UNIVERSAL_COMMAND_H_
