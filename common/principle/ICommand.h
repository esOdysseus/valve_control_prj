#ifndef _INTERFACE_COMMAND_H_
#define _INTERFACE_COMMAND_H_

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


class ICommand {
public:
    using Twho = principle::CWho;
    using Twhen = principle::CWhen;
    using Twhere = principle::CWhere;
    using Twhat = principle::CWhat;
    using Thow = principle::CHow;
    using Twhy = std::string;
    using FlagType = uint8_t;
    static constexpr const char* PROTOCOL_NAME = "none";

private:
    static constexpr const char* NAME = "defCMD";
    static constexpr const char* VERSION = "1.0.0";

public:
    ICommand(std::string my_app_path, std::string my_pvd_id);

    ICommand(const alias::CAlias& myself, FlagType flag_val);

    virtual ~ICommand(void);

    virtual std::string name(void) const { return NAME; };

    virtual std::string proto_name(void) const { return PROTOCOL_NAME; };

    std::string version(void) const { return VERSION; };

    bool is_parsed(void) const { return _is_parsed_; }

    // presentator
    virtual bool decode(std::shared_ptr<IProtocolInf>& protocol);

    virtual std::shared_ptr<payload::CPayload> encode( std::shared_ptr<ICommunicator>& handler );

    // getter
    virtual unsigned long get_id(void) {    return 0;   };

    virtual FlagType get_flag(FlagType pos=common::E_FLAG::E_FLAG_ALL) { return common::E_FLAG::E_FLAG_NONE; };

    const alias::CAlias& get_from(void) const { return _myself_from_; }

    double get_rcv_time(void) const { return _rcv_time_; }

    // setter
    void set_when( std::string type, double start_time, 
                                     Twhen::TEweek week = Twhen::TEweek::E_WEEK_NONE, 
                                     uint32_t period = Twhen::PERIOD_NULL, 
                                     double latency = Twhen::LATENCY_NULL );

    void set_how( std::string method, std::string post_method = Thow::METHOD_NULL, 
                                      double costtime = Thow::COSTTIME_NULL );

    // compare time
    E_CMPTIME compare_with_curtime(double duty=1.0);   // check whether cmd-time is over/under/equal corespond to current-time.

    E_CMPTIME compare_with_another(ICommand *cmd, double duty=1.0);   // check whether cmd-time is over/under/equal corespond to another cmd-time.

    /***
     * Principle-6
     */
    Twho& who(void);

    Twhen& when(void);

    Twhere& where(void);

    Twhat& what(void);

    Thow& how(void);

    Twhy& why(void);

protected:
    void set_flag_parse( bool value, double rcv_time=0.0 );

    std::string extract_version(Json_DataType &json);

    std::shared_ptr<Twho> extract_who(Json_DataType &json);

    std::shared_ptr<Twhen> extract_when(Json_DataType &json, double def_time=0.0);

    std::shared_ptr<Twhere> extract_where(Json_DataType &json);

    std::shared_ptr<Twhat> extract_what(Json_DataType &json);

    std::shared_ptr<Thow> extract_how(Json_DataType &json);

    std::shared_ptr<Twhy> extract_why(Json_DataType &json);

    bool apply_version(Json_DataType &json);

    bool apply_who(Json_DataType &json, std::shared_ptr<Twho>& value);

    bool apply_when(Json_DataType &json, std::shared_ptr<Twhen>& value);

    bool apply_where(Json_DataType &json, std::shared_ptr<Twhere>& value);

    bool apply_what(Json_DataType &json, std::shared_ptr<Twhat>& value);

    bool apply_how(Json_DataType &json, std::shared_ptr<Thow>& value);

    bool apply_why(Json_DataType &json, std::shared_ptr<Twhy>& value);

private:
    ICommand(void) = delete;

    void clear(void);

protected:
    // Principle-6: Who, When, Where, What, How, Why
    std::shared_ptr<Twho> _who_;

    std::shared_ptr<Twhen> _when_;

    std::shared_ptr<Twhere> _where_;

    std::shared_ptr<Twhat> _what_;

    std::shared_ptr<Thow> _how_;

    std::shared_ptr<Twhy> _why_;

private:
    // Data-Structure for Encoded packet.
    bool _is_parsed_;     // If data is encoded, then set 'false'. vice versa set 'true'.

    // packet received time
    double _rcv_time_;          // I receive this packet in time.

    // Data-Structure for Decoded packet.
    alias::CAlias _myself_from_;

};


}   // namespace cmd


#endif // _INTERFACE_COMMAND_H_
