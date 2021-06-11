#ifndef _VALVE_CONTROL_COMMAND_H_
#define _VALVE_CONTROL_COMMAND_H_

#include <ctime>
#include <string>
#include <memory>

#include <json_manipulator.h>
#include <ICommunicator.h>
#include <IProtocolInf.h>
#include <Common.h>


/*******************************
 * Payload-format
 ******************************/
/*
{   // following is format for body.
    'where': {
        'type': 'center',
        'gps': {                // type 이란 장소의 중심 좌표만 기록한다.
            'long': 'double',
            'lat': 'double'
        }
    },
    'what': {
        'type': 'valve-swc',
        'seq': '1'
    },
    'how': {
        'method': 'open',       // open , close
        'costtime': 'double'    // seconds + point value
                                // -1.0 : when method is close
    },
    'why': {
        'desp': 'string',
        'objective': ['key-words', ... ],
        'dependency': ['key-words', ... ]
    }
}
*/

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

typedef struct _time_spec_ {
    int64_t tv_sec;
    int64_t tv_nsec;
} _time_spec_;


/*******************************
 * Definition of Class.
 */
namespace valve_pkg {

typedef enum E_WEEK {
    E_WEEK_MONDAY = 0,
    E_WEEK_TUESDAY = 1,
    E_WEEK_WEDNESDAY = 2,
    E_WEEK_THURSDAY = 3,
    E_WEEK_FRIDAY = 4,
    E_WEEK_SATURDAY = 5,
    E_WEEK_SUNDAY = 6
} E_WEEK;

typedef enum E_CMPTIME {
    E_CMPTIME_UNKNOWN = -1,
    E_CMPTIME_EQUAL = 0,        // within +- 1 seconds.
    E_CMPTIME_OVER = 1,
    E_CMPTIME_UNDER = 2
} E_CMPTIME;

class CCommand {
public:
    using FlagType = uint8_t;
    using E_TARGET = enum E_TARGET {
        E_VALVE_NULL = -1,
        E_VALVE_LEFT_01 = 0,
        E_VALVE_LEFT_02 = 1,
        E_VALVE_LEFT_03 = 2,
        E_VALVE_LEFT_04 = 3,
        E_VALVE_CNT = 4
    };

    static constexpr const char* PROTOCOL_NAME = "CPUniversalCMD";
    static constexpr const char* OPEN = "open";
    static constexpr const char* CLOSE = "close";

private:
    class CVwhat;
    class CVhow;
    using Twhat = std::shared_ptr<CVwhat>;
    using Thow = std::shared_ptr<CVhow>;
    using Twhy = std::string;

public:
    CCommand(std::string my_app_path, std::string my_pvd_id);

    CCommand(alias::CAlias& myself, const CCommand *cmd, FlagType flag_val);

    CCommand( alias::CAlias& myself, FlagType flag_val);

    ~CCommand(void);

    void clear(void);

    // presentator
    bool decode(std::shared_ptr<IProtocolInf>& protocol);

    std::shared_ptr<payload::CPayload> encode( std::shared_ptr<ICommunicator>& handler );

    // setter
    void set_id(unsigned long value) { 
        _msg_id_ = value;
        if( _msg_id_ == 0 ) {
            _msg_id_ = gen_random_msg_id();
        }
    }

    void set_flag(E_FLAG pos, FlagType value=0);

    void set_state(uint16_t value);

    void set_from(alias::CAlias &value) { _myself_from_ = value; }

    bool set_when(void);    // set current-time to command.

    bool set_when(E_WEEK week);  // set next week-day base on cmd-time. (hour/minute/sec is zero-set.)

    bool set_when(int hour, int minute, int seconds);   // set next time base on cmd-time.

    bool set_when(int year, int month, int day, int hour, int minute, int seconds);   // set next time base on current-time.

    bool append_when(int hour, int minute, int seconds);

    // getter
    bool parsing_complet(void) { return _is_parsed_; }

    unsigned long get_id(void) { return _msg_id_; }

    FlagType get_flag(FlagType pos=E_FLAG::E_FLAG_ALL);

    uint16_t get_state(void);

    alias::CAlias get_from(void) { return _myself_from_; }

    template <typename _OUT_TYPE_=double>
    _OUT_TYPE_ get_when(void);

    double get_rcv_packet_time(void) { return _rcv_time_; }

    double get_cur_time(void);

    E_TARGET get_what_valve(void);

    std::string get_how_method(void);

    double get_how_costtime(void);

    std::string get_why(void) { return _why_; }

    // printer
    std::string print_when(void);  // print when-data for human-readable.

    static std::string print_cur_time(void);  // print current-time for human-readable.

    // compare time
    E_CMPTIME check_with_curtime(double duty=1.0);   // check whether cmd-time is over/under/equal corespond to current-time.

    E_CMPTIME check_with_another(CCommand *cmd, double duty=1.0);   // check whether cmd-time is over/under/equal corespond to another cmd-time.

private:
    E_WEEK get_week(struct tm &time);

    double extract_utc2double(struct timespec &time);

    Twhat extract_what(Json_DataType &json);

    Thow extract_how(Json_DataType &json);

    Twhy extract_why(Json_DataType &json);

    bool apply_double2utc(double &time, struct timespec &target);

    bool apply_what(Json_DataType &json, Twhat& valve);

    bool apply_how(Json_DataType &json, Thow& value);

    bool apply_why(Json_DataType &json, Twhy& value);

    uint32_t gen_random_msg_id(void);

private:
    // Data-Structure for Encoded packet.
    bool _is_parsed_;     // If data is encoded, then set 'false'. vice versa set 'true'.

    // Data-Structure for Decoded packet.
    uint8_t _flag_;
    
    uint16_t _state_;

    uint32_t _msg_id_;

    alias::CAlias _myself_from_;

    tm _when_tm_;            // parsed time (min-unit is seconds)

    double _when_double_;    // UTC time with nano-seconds.

    Twhat _what_;          // 0 ~ 3

    Thow _how_;        // 'open' or 'close'

    Twhy _why_;        // option : simple string.

    double _rcv_time_;        // I receive this packet in time(_rcv_time_).

};

}   // namespace valve_pkg


#endif // _VALVE_CONTROL_COMMAND_H_
