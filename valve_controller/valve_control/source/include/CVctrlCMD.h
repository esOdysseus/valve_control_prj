#ifndef _VALVE_CONTROL_COMMAND_H_
#define _VALVE_CONTROL_COMMAND_H_

#include <ctime>
#include <string>

#include <json_manipulator.h>

#define WHO_CAP     19
#define SOF_        "UCMD"

/*******************************
 * Payload-format
 ******************************/
//  sof(4 byte)    //
//  flag(1 byte)   //
//  reserve(1 byte)//
//  state(2 byte)  //
//  MSG_ID(4 byte) //
//  from(20 byte)  //
//  who(20 byte)   //
//  when(8 byte)   //
//  length(4 byte) //
//  body(dynamic)  //
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
        'method': 'open'    // open , close
    },
    'why': {
        'desp': 'string',
        'objective': ['key-words', ... ],
        'dependency': ['key-words', ... ]
    }
}
*/

typedef uint8_t headerFlagType;

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

#pragma pack(push, 1)
typedef struct header_st {
    char sof[4];
    headerFlagType flag : 8;
    uint32_t reserved   : 8;
    uint32_t state      : 16;   // describe about state-error.
    uint32_t MSG_ID     : 32;
    char from[WHO_CAP+1];       // sent-app name
    char who[WHO_CAP+1];        // receiving-app name. (string)
    _time_spec_ when;           // for time-driven task-scheduling.
    uint32_t length;            // size of body. (where, what, how, why)
} header_st;
#pragma pack(pop)


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

class CVctrlCMD {
public:
    using FlagType = headerFlagType;
    using E_TARGET = enum E_TARGET {
        E_VALVE_NULL = -1,
        E_VALVE_LEFT_01 = 0,
        E_VALVE_LEFT_02 = 1,
        E_VALVE_LEFT_03 = 2,
        E_VALVE_LEFT_04 = 3,
        E_VALVE_CNT = 4
    };

    static constexpr char* OPEN = "open";
    static constexpr char* CLOSE = "close";

public:
    CVctrlCMD(std::string myself_name);

    CVctrlCMD(std::string myself_name, 
              const CVctrlCMD *cmd, 
              std::string target, 
              FlagType flag_val);

    CVctrlCMD(std::string myself_name,
              std::string target_app,
              FlagType flag_val);

    ~CVctrlCMD(void);

    void clear(void);

    // presentator
    bool decode(const void * payload, ssize_t payload_size);

    void * encode(ssize_t &size);

    // setter
    void set_id(unsigned long value=0) { 
        msg_id = value;
        if( msg_id == 0 ) {
            msg_id = gen_random_msg_id();
        }
    }

    void set_flag(E_FLAG pos, FlagType value=0);

    void set_state(uint16_t value);

    void set_from(std::string &value) { from = value; }

    void set_who(std::string &value) { who = value; }

    bool set_when(void);    // set current-time to command.

    bool set_when(E_WEEK week);  // set next week-day base on cmd-time. (hour/minute/sec is zero-set.)

    bool set_when(int hour, int minute, int seconds);   // set next time base on cmd-time.

    bool set_when(int year, int month, int day, int hour, int minute, int seconds);   // set next time base on current-time.

    bool append_when(int hour, int minute, int seconds);

    bool set_what(E_TARGET valve);

    bool set_how(const std::string &method);

    bool set_why(const std::string &desp);

    // getter
    std::string get_sof(const void *payload, ssize_t payload_size);

    bool parsing_complet(void) { return is_parsed; }

    unsigned long get_id(void) { return msg_id; }

    FlagType get_flag(FlagType pos=E_FLAG::E_FLAG_ALL);

    uint16_t get_state(void);

    std::string get_from(void) { return from; }

    std::string get_who(void) { return who; }

    template <typename _OUT_TYPE_=double>
    _OUT_TYPE_ get_when(void);

    double get_rcv_packet_time(void) { return _rcv_time_; }

    double get_cur_time(void);

    E_TARGET get_what(void) { return what; }

    std::string get_how(void) { return how; }

    std::string get_why(void) { return why; }

    // printer
    std::string print_when(void);  // print when-data for human-readable.

    static std::string print_cur_time(void);  // print current-time for human-readable.

    // compare time
    E_CMPTIME check_with_curtime(double duty=1.0);   // check whether cmd-time is over/under/equal corespond to current-time.

    E_CMPTIME check_with_another(CVctrlCMD *cmd, double duty=1.0);   // check whether cmd-time is over/under/equal corespond to another cmd-time.

private:
    bool check_sof_for_decode(const void *payload, ssize_t payload_size);

    E_WEEK get_week(struct tm &time);

    double extract_utc2double(struct timespec &time);

    E_TARGET extract_what(Json_DataType &json);

    std::string extract_how(Json_DataType &json);

    std::string extract_why(Json_DataType &json);

    bool apply_double2utc(double &time, struct timespec &target);

    bool apply_what(Json_DataType &json, E_TARGET valve_seq);

    bool apply_how(Json_DataType &json, std::string desp);

    bool apply_why(Json_DataType &json, std::string desp);

    uint32_t gen_random_msg_id(void);

private:
    // Data-Structure for Encoded packet.
    header_st _header_;     // metadata

    bool is_parsed;     // If data is encoded, then set 'false'. vice versa set 'true'.

    // Data-Structure for Decoded packet.
    uint8_t _flag_;
    
    uint16_t _state_;

    uint32_t msg_id;

    std::string from;

    std::string who;

    tm when_tm;            // parsed time (min-unit is seconds)

    double when_double;    // UTC time with nano-seconds.

    E_TARGET what;          // 0 ~ 3

    std::string how;        // 'open' or 'close'

    std::string why;        // option : simple string.

    double _rcv_time_;        // I receive this packet in time(_rcv_time_).

};

}   // namespace valve_pkg


#endif // _VALVE_CONTROL_COMMAND_H_
