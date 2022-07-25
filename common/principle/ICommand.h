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
        'type': 'string',           // valid-values : [ 'center.gps', 'unknown', 'dont.care', 'db' ]
        'contents': {               // center.gps : 일때, 장소의 중심 좌표만 기록한다.
            'long': 'double',
            'lat': 'double'
        }
        'contents': {               // db : DataBase의 type/path/table 로 정확한 DB상의 위치를 나타낸다.
            'type': 'SQL',          // valid-values : [ 'SQL', 'NOSQL' ]
            'path': '/db/path/db-test.db',
            'table': 'DB_TABLE_EVENT',
        }
    },
    'what': {
        'type': 'string',           // valid-values : [ 'valve.swc', 'db', 'req.cmd', 'resp.cmd' ]
        'contents': {               // valve.swc : 일때, 어떤 switch를 선택할지를 나타낸다.
            'seq': 'uint32_t'
        }
        'contents': {               // db : DataBase에 Insert/Update//Select/Delete 할 대상을 명시한다.
                                    //      Select/Delete에선, 대상이 없으므로 'none'이 된다.
            'type': 'elements',     // valid-values : [ 'records', 'elements', 'none' ]
            'target': {
                '1': {
                    'key01': 'value01',
                    'key02': 'value02'
                }
            }
        }
        'contents': {               // req.cmd : REQ를 보내는 입장에서, method 수행자가 일관되게 수행할수 있도록 고민한다.
                                    //           명료성과, 일관된 format을 기반으로 제한적 Description이 중요해 보인다.
            'type': 'future',       // valid-values : [ 'future', 'now', 'past' ] DataBase의 type을 나타낸다.
            'table': 'event',       // valid-values : [ 'event', 'period' ]  DB의 Table-name을 넣는다.
            'cmds': {
                'needs': [ 'key01', 'key02' ]
            }
        }
        'contents': {               // resp.cmd : REQ에 대한 응답으로 어떤 내용을 요청자에게 제공할지에 초점을 맞춘다.
            'type': 'future',       // valid-values : [ 'future', 'now', 'past' ] DataBase의 type을 나타낸다.
            'table': 'event',       // valid-values : [ 'event', 'period' ]  DB의 Table-name을 넣는다.
            'cmds': {
                '${uuid}': {        // 기본적으로 모든 CMD는 uuid를 가지며, indexing이 가능하므로 기본 제공된다.
                    'key01': 'value01',
                    'key02': 'value02',
                    'error': 'no error' // It describe error-state with error message as text.
                }
            },
            'result': {
                'method': 'get',    // valid-values : [ 'get', 'put', 'delete', 'disable', 'enable' ]
                'state': 'OK'       // valid-values : [ 'OK', 'FAIL' ]  cmd들 모두가 성공하면, 최종 OK로 설정한다.
            }
        }
    },
    'how': {
        'type': 'string',           // valid-values : [ 'valve.swc', 'db', 'req.cmd' ]
        'contents': {               // valve.swc : 일때, Action의 Start~Stop까지 묶어준다.
            'method-pre': 'open',   // valid-values : [ open , close, none ]
            'costtime': 'double',   // valid-values : seconds + point value
                                    //                0.0 : when method is close
            'method-post': 'close'  // valid-values : [ open , close, none ]
        }
        'contents': {               // db : DataBase에 what을 어떻게 적용할지를 명시한다.
            'method': 'update',     // valid-values : [ 'select', 'delete', 'insert', 'update' ]
            'condition': {
                '1': 'string'       // 'string' 한개는 What의 'contents.target' 1개와 동일한 key 위치에서 pair가 된다.
            }
        }
        'contents': {               // req.cmd : DataBase에 what을 어떻게 적용할지를 명시한다.
            'method': 'get',        // valid-values : [ 'get', 'put', 'delete', 'disable', 'enable' ]
            'condition': {          // valid-keys : [ 'uuid', 'from-date', 'to-date' ]
                                    //              uuid와 (from-date, to-date)는 XOR 관계이다.
                'uuid' : [ 'xxxxx' ],
                'from-date' : '2022-05-13 13:00:00',
                'to-date' : '2022-05-13 13:30:00'
            }
        }
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
    virtual uint32_t get_id(void) {    return 0;   };

    virtual FlagType get_flag(FlagType pos=common::E_FLAG::E_FLAG_ALL) { return common::E_FLAG::E_FLAG_NONE; };

    const alias::CAlias& get_from(void) const { return _myself_from_; }

    double get_rcv_time(void) const { return _rcv_time_; }

    const std::string& get_payload(void) const { return _payload_; }

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

    static std::string extract_version(Json_DataType &json);

    static std::shared_ptr<Twho> extract_who(Json_DataType &json);

    static std::shared_ptr<Twhen> extract_when(Json_DataType &json, double def_time=0.0);

    static std::shared_ptr<Twhere> extract_where(Json_DataType &json);

    static std::shared_ptr<Twhat> extract_what(Json_DataType &json);

    static std::shared_ptr<Thow> extract_how(Json_DataType &json);

    static std::shared_ptr<Twhy> extract_why(Json_DataType &json);

    static bool apply_version(Json_DataType &json);

    static bool apply_who(Json_DataType &json, std::shared_ptr<Twho>& value);

    static bool apply_when(Json_DataType &json, std::shared_ptr<Twhen>& value);

    static bool apply_where(Json_DataType &json, std::shared_ptr<Twhere>& value);

    static bool apply_what(Json_DataType &json, std::shared_ptr<Twhat>& value);

    static bool apply_how(Json_DataType &json, std::shared_ptr<Thow>& value);

    static bool apply_why(Json_DataType &json, std::shared_ptr<Twhy>& value);
    
protected:
    void set_flag_parse( bool value, double rcv_time=0.0 );

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

    std::string _payload_;      // json-data of payload

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
