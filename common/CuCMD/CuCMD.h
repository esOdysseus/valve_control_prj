#ifndef _UNIVERSAL_COMMAND_H_
#define _UNIVERSAL_COMMAND_H_

#include <ctime>
#include <string>
#include <memory>

#include <ICommand.h>
#include <Common.h>



/*******************************
 * Definition of Class.
 */
namespace cmd {


class CuCMD: public ICommand {
public:
    using E_FLAG = common::E_FLAG;
    using E_STATE = common::E_STATE;
    static constexpr const char* NAME = "uCMD";
    static constexpr const char* PROTOCOL_NAME = "CPUniversalCMD";

public:
    CuCMD(std::string my_app_path, std::string my_pvd_id);

    CuCMD( alias::CAlias& myself, FlagType flag_val);

    CuCMD( const CuCMD& cmd );

    ~CuCMD(void);

    std::string name(void) const override { return NAME; }

    std::string proto_name(void) const override { return PROTOCOL_NAME; }

    // presentator
    bool decode(std::shared_ptr<IProtocolInf>& protocol) override;

    std::shared_ptr<payload::CPayload> encode( std::shared_ptr<ICommunicator>& handler ) override;

    static std::shared_ptr<payload::CPayload> force_encode( std::shared_ptr<ICommunicator>& handler, 
                                                            std::string payload, 
                                                            FlagType flag, common::StateType state, uint32_t& msg_id );

    // getter
    uint32_t get_id(void) override { return _msg_id_; }

    FlagType get_flag(FlagType pos=E_FLAG::E_FLAG_ALL) override;

    uint16_t get_state(void) { return _state_; }

    double get_send_time(void) { return _send_time_d_; }

    // setter
    void set_id(uint32_t value);

    void set_flag(E_FLAG pos, FlagType value=0);

    void set_state(uint16_t value);

    // printer
    std::string print_send_time(void);  // print when-data for human-readable.

private:
    void clear(void);

    static uint32_t gen_random_msg_id(void);

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
