#ifndef _UNIVERSAL_COMMAND_H_
#define _UNIVERSAL_COMMAND_H_

#include <ctime>
#include <string>
#include <memory>

#include <json_manipulator.h>
#include <ICommunicator.h>
#include <IProtocolInf.h>
#include <Common.h>
#include <Principle6.h>


/*******************************
 * Command(CMD)-format
 ******************************/
/*
{   // following is payload format.
    'version': '1.0.0',
    'who': {
        'app': 'string',            // APP-path in alias
        'pvd': 'string',            // Provider-id in alias
        'func': 'string'            // Optional (for Service-oriented)
    },
    'when': {
        'type': 'string',           // valid-values : [ one-time, routine.week, routine.day, specific ]
        'time': {
            'latency': 'double'     // unit : second  (for one-time)
            'week': 'string'        // valid-values : (for routine.week) [ mon, tues, wednes, thurs, fri, satur, sun ]
            'period' : 'uint32_t'   // valid-values : (for routine) [ 1 <= X ]: 2 week/day routine ...
            'date' : '21-06-13'     // valid-values : (for routine & specific)
            'time' : '15:30:23'     // valid-values : (for routine & specific)
        }
    },
    'where': {
        'type': 'string',           // valid-values : [ 'center.gps', 'unknown' ]
        'gps': {                    // center.gps일때, 장소의 중심 좌표만 기록한다.
            'long': 'double',
            'lat': 'double'
        }
    },
    'what': {
        'type': 'string',           // valid-values : [ 'valve.swc' ]
        'seq': 'uint32_t'           // valve.swc일때, 어떤 switch를 선택할지를 나타낸다.
    },
    'how': {
        'method-pre': 'open',       // valid-values : [ open , close ]
        'costtime': 'double',       // valid-values : seconds + point value
                                    //                0.0 : when method is close
        'method-post': 'close'      // valid-values : [ open , close, none ]
    },
    'why': {                        // Optional
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


/*******************************
 * Definition of Class.
 */
namespace cmd {

typedef enum E_CMPTIME {
    E_CMPTIME_UNKNOWN = -1,
    E_CMPTIME_EQUAL = 0,        // within +- 1 seconds.
    E_CMPTIME_OVER = 1,
    E_CMPTIME_UNDER = 2
} E_CMPTIME;


class CCommand {
public:
    using FlagType = uint8_t;
    static constexpr const char* PROTOCOL_NAME = "CPUniversalCMD";

public:
    using Twho = principle::CWho;
    using Twhen = principle::CWhen;
    using Twhere = principle::CWhere;
    using Twhat = principle::CWhat;
    using Thow = principle::CHow;
    using Twhy = std::string;

public:
    CCommand(std::string my_app_path, std::string my_pvd_id);

    CCommand( alias::CAlias& myself, FlagType flag_val);

    CCommand( const CCommand& cmd );

    ~CCommand(void);

    void clear(void);

    // presentator
    bool decode(std::shared_ptr<IProtocolInf>& protocol);

    std::shared_ptr<payload::CPayload> encode( std::shared_ptr<ICommunicator>& handler );

    bool parsing_complet(void) { return _is_parsed_; }

    // getter
    unsigned long get_id(void) { return _msg_id_; }

    FlagType get_flag(FlagType pos=E_FLAG::E_FLAG_ALL);

    uint16_t get_state(void);

    alias::CAlias get_from(void) { return _myself_from_; }

    double get_rcv_time(void) { return _rcv_time_; }

    double get_send_time(void) { return _send_time_d_; }

    // setter
    void set_id(unsigned long value) { 
        _msg_id_ = value;
        if( _msg_id_ == 0 ) {
            _msg_id_ = gen_random_msg_id();
        }
    }

    void set_flag(E_FLAG pos, FlagType value=0);

    void set_state(uint16_t value);

    void set_when( std::string type, double start_time, 
                                     Twhen::TEweek week = Twhen::TEweek::E_WEEK_NONE, 
                                     uint32_t period = Twhen::PERIOD_NULL, 
                                     double latency = Twhen::LATENCY_NULL );

    void set_how( std::string method, std::string post_method = Thow::METHOD_NULL, 
                                      double costtime = Thow::COSTTIME_NULL );

    // printer
    std::string print_send_time(void);  // print when-data for human-readable.

    /***
     * Principle-6
     */
    Twho& who(void);

    Twhen& when(void);

    Twhere& where(void);

    Twhat& what(void);

    Thow& how(void);

    Twhy& why(void);

    // compare time
    E_CMPTIME check_with_curtime(double duty=1.0);   // check whether cmd-time is over/under/equal corespond to current-time.

    E_CMPTIME check_with_another(CCommand *cmd, double duty=1.0);   // check whether cmd-time is over/under/equal corespond to another cmd-time.

private:
    // std::shared_ptr<Twho> extract_who(Json_DataType &json);

    // std::shared_ptr<Twhen> extract_when(Json_DataType &json);

    // std::shared_ptr<Twhere> extract_where(Json_DataType &json);

    std::shared_ptr<Twhat> extract_what(Json_DataType &json);

    std::shared_ptr<Thow> extract_how(Json_DataType &json);

    std::shared_ptr<Twhy> extract_why(Json_DataType &json);

    // bool apply_who(Json_DataType &json, std::shared_ptr<Twho>& valve);

    // bool apply_when(Json_DataType &json, std::shared_ptr<Twhen>& valve);

    // bool apply_where(Json_DataType &json, std::shared_ptr<Twhere>& valve);

    bool apply_what(Json_DataType &json, std::shared_ptr<Twhat>& valve);

    bool apply_how(Json_DataType &json, std::shared_ptr<Thow>& value);

    bool apply_why(Json_DataType &json, std::shared_ptr<Twhy>& value);

    uint32_t gen_random_msg_id(void);

private:
    // Data-Structure for Encoded packet.
    bool _is_parsed_;     // If data is encoded, then set 'false'. vice versa set 'true'.

    // Data-Structure for Decoded packet.
    uint8_t _flag_;
    
    uint16_t _state_;

    uint32_t _msg_id_;

    alias::CAlias _myself_from_;

    // send/receive time
    struct tm _send_time_tm_;          // parsed time (min-unit is seconds)

    double _send_time_d_;       // UTC time with nano-seconds.

    double _rcv_time_;          // I receive this packet in time(_rcv_time_).

    // Principle-6: Who, When, Where, What, How, Why
    std::shared_ptr<Twho> _who_;

    std::shared_ptr<Twhen> _when_;

    std::shared_ptr<Twhere> _where_;

    std::shared_ptr<Twhat> _what_;

    std::shared_ptr<Thow> _how_;

    std::shared_ptr<Twhy> _why_;

};


}   // namespace cmd


#endif // _UNIVERSAL_COMMAND_H_
