#include <cassert>
#include <random>
#include <memory>
#include <iostream>
#include <string>

#include <logger.h>
#include <ICommand.h>
#include <Common.h>
#include <CException.h>
#include <time_kes.h>


namespace cmd {

constexpr const char* ICommand::VERSION;

static std::string JKEY_VERSION            = "version";
static std::string JKEY_WHO                = "who";
static std::string JKEY_WHO_APP            = "app";
static std::string JKEY_WHO_PVD            = "pvd";
static std::string JKEY_WHO_FUNC           = "func";
static std::string JKEY_WHEN               = "when";
static std::string JKEY_WHEN_TYPE          = "type";
static std::string JKEY_WHEN_TIME          = "time";
static std::string JKEY_WHEN_LATENCY       = "latency";
static std::string JKEY_WHEN_WEEK          = "week";
static std::string JKEY_WHEN_PERIOD        = "period";
static std::string JKEY_WHEN_DATE          = "date";
static std::string JKEY_WHERE              = "where";
static std::string JKEY_WHERE_TYPE         = "type";
static std::string JKEY_WHERE_CONTENTS     = "contents";
static std::string JKEY_WHERE_GPS_LONG     = "long";
static std::string JKEY_WHERE_GPS_LAT      = "lat";
static std::string JKEY_WHAT               = "what";
static std::string JKEY_WHAT_TYPE          = "type";
static std::string JKEY_WHAT_CONTENTS      = "contents";
static std::string JKEY_WHAT_SEQ           = "seq";
static std::string JKEY_HOW                = "how";
static std::string JKEY_HOW_TYPE           = "type";
static std::string JKEY_HOW_CONTENTS       = "contents";
static std::string JKEY_HOW_METHOD         = "method-pre";
static std::string JKEY_HOW_METHOD_POST    = "method-post";
static std::string JKEY_HOW_COSTTIME       = "costtime";
static std::string JKEY_WHY                = "why";
static std::string JKEY_WHY_DESP           = "desp";
static std::string JKEY_WHY_OBJ            = "objective";
static std::string JKEY_WHY_DEP            = "dependency";


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


/**********************************
 * Definition of Public Function.
 */
ICommand::ICommand(std::string my_app_path, std::string my_pvd_id)
: _myself_from_( my_app_path, my_pvd_id ) {
    clear();
    assert(_myself_from_.empty() == false);
}

ICommand::ICommand(const alias::CAlias& myself, FlagType flag_val)
: _myself_from_( myself ) {
    clear();
    assert(_myself_from_.empty() == false);
}

ICommand::~ICommand(void) {
    clear();
}

bool ICommand::decode(std::shared_ptr<IProtocolInf>& protocol) {
    size_t payload_size = 0;
    if( protocol.get() == NULL ) {
        LOGW("Protocol is empty.");
        return false;
    }

    try {
        Json_DataType json_manager;
        const char* payload = (const char*)protocol->get_payload(payload_size);
        if( payload == NULL ) {
            LOGERR("Payload(0x%X) is NULL or length(%u) < 0.", payload, payload_size);
            throw std::invalid_argument("Payload is NULL or length <= 0.");
        }

        if( is_parsed() == false) {
            // parsing json payload (where, what, how, why)
            json_manager = std::make_shared<json_mng::CMjson>();
            LOGD("payload=%s , length=%d", payload, payload_size);
            if( json_manager->parse(payload, payload_size) != true) {
                throw std::runtime_error("Invalid Json-payload. Please check it.");
            }
            _payload_ = std::string( json_manager->print_buf() );
            LOGD("_payload_=%s", _payload_.data());

            // check UniversalCMD version.
            auto ver = extract_version(json_manager);
            if( ver != version() ) {
                std::string err = "VERSION(" + ver + ") of json-context != " + version();
                throw std::invalid_argument(err);
            }
            // parse principle-6.
            _who_ = extract_who(json_manager);
            _when_ = extract_when(json_manager);
            _where_ = extract_where(json_manager);
            _what_ = extract_what(json_manager);
            _how_ = extract_how(json_manager);
            _why_ = extract_why(json_manager);
            LOGD( "Success parse of Json buffer." );

            // mark receive-time of this packet using my-system time.
            set_flag_parse( true );
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        _payload_.clear();
        throw CException(E_ERROR::E_ERR_FAIL_DECODING_CMD);
    }

    return is_parsed();
}

std::shared_ptr<payload::CPayload> ICommand::encode( std::shared_ptr<ICommunicator>& handler ) {
    const char* body = NULL;
    std::shared_ptr<payload::CPayload> message;

    if( handler.get() == NULL ) {
        LOGW("Communicator is not exist.");
        return message;
    }

    try {
        Json_DataType json_manager;
        json_manager = std::make_shared<json_mng::CMjson>();
        message = handler->create_payload();
        if( message.get() == NULL ) {
            throw std::logic_error("Message-Creating is failed.");
        }

        // set UniversalCMD version.
        assert(apply_version(json_manager) == true);
        // set principle-6.
        assert(apply_who(json_manager, _who_) == true);
        assert(apply_when(json_manager, _when_) == true);
        assert(apply_where(json_manager, _where_) == true);
        assert(apply_what(json_manager, _what_) == true);
        assert(apply_how(json_manager, _how_) == true);
        assert(apply_why(json_manager, _why_) == true);
        assert( (body = json_manager->print_buf()) != NULL );

        message->set_payload( body, strlen(body) );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        message.reset();
        throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
    }

    return message;
}


void ICommand::set_when( std::string type, double start_time, 
                                           Twhen::TEweek week, 
                                           uint32_t period, 
                                           double latency ) {
    _when_.reset();
    _when_ = std::make_shared<Twhen>(type, start_time, week, period, latency);
}

void ICommand::set_how( std::string method, std::string post_method, double costtime ) {
    try {
        _how_.reset();
        auto method_pre = principle::type_convert<principle::Tvalve_method>(method);
        auto method_post = principle::type_convert<principle::Tvalve_method>(post_method);
        _how_ = std::make_shared<Thow>(Thow::TYPE_VALVE, method_pre, costtime, method_post);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

// compare time
E_CMPTIME ICommand::compare_with_curtime(double duty) {   // check whether current-time is over/under/equal with cmd-time.
    try {
        if (  is_parsed() == false ) {
            throw std::logic_error("Command-parsing is not processed.");
        }

        double d_now = 0.0;
        double run_time = when().get_start_time();

        // get current-time by REAL-TIME-CLOCK.
        d_now = time_pkg::CTime::get<double>();

        if ( run_time < (d_now - duty) ) {
            return E_CMPTIME::E_CMPTIME_UNDER;
        }
        else if( run_time > (d_now + duty) ) {
            return E_CMPTIME::E_CMPTIME_OVER;
        }
        else {
            return E_CMPTIME::E_CMPTIME_EQUAL;
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return E_CMPTIME::E_CMPTIME_UNKNOWN;
}

E_CMPTIME ICommand::compare_with_another(ICommand *cmd, double duty) {
    assert( cmd != NULL );
    try {
        if (  is_parsed() == false ) {
            throw std::logic_error("Command-parsing is not processed.");
        }

        double d_another = cmd->when().get_start_time();
        double run_time = when().get_start_time();

        if ( run_time < (d_another - duty) ) {
            return E_CMPTIME::E_CMPTIME_UNDER;
        }
        else if( run_time > (d_another + duty) ) {
            return E_CMPTIME::E_CMPTIME_OVER;
        }
        else {
            return E_CMPTIME::E_CMPTIME_EQUAL;
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return E_CMPTIME::E_CMPTIME_UNKNOWN;
}

ICommand::Twho& ICommand::who(void) { 
    if( is_parsed() == false ) {
        throw std::logic_error("Twho Parsing is not complete.");
    }
    if( _who_.get() == NULL ) {
        throw std::out_of_range("Twho is NULL.");
    }
    return *_who_; 
}

ICommand::Twhen& ICommand::when(void) { 
    if( is_parsed() == false ) {
        throw std::logic_error("Twhen Parsing is not complete.");
    }
    if( _when_.get() == NULL ) {
        throw std::out_of_range("Twhen is NULL.");
    }
    return *_when_; 
}

ICommand::Twhere& ICommand::where(void) { 
    if( is_parsed() == false ) {
        throw std::logic_error("Twhere Parsing is not complete.");
    }
    if( _where_.get() == NULL ) {
        throw std::out_of_range("Twhere is NULL.");
    }
    return *_where_; 
}

ICommand::Twhat& ICommand::what(void) { 
    if( is_parsed() == false ) {
        throw std::logic_error("Twhat Parsing is not complete.");
    }
    if( _what_.get() == NULL ) {
        throw std::out_of_range("Twhat is NULL.");
    }
    return *_what_; 
}

ICommand::Thow& ICommand::how(void) { 
    if( is_parsed() == false ) {
        throw std::logic_error("Thow Parsing is not complete.");
    }
    if( _how_.get() == NULL ) {
        throw std::out_of_range("Thow is NULL.");
    }
    return *_how_; 
}

ICommand::Twhy& ICommand::why(void) { 
    if( is_parsed() == false ) {
        throw std::logic_error("Twhy Parsing is not complete.");
    }
    if( _why_.get() == NULL ) {
        throw std::out_of_range("Twhy is NULL.");
    }
    return *_why_; 
}


/***********************************
 * Definition of Protected Function.
 */
void ICommand::set_flag_parse( bool value, double rcv_time ) {
    _is_parsed_ = value;
    _rcv_time_ = rcv_time;

    if( _is_parsed_ == true && _rcv_time_ == 0.0 ) {
        _rcv_time_ = time_pkg::CTime::get<double>();    // get current time.
    }
}

std::string ICommand::extract_version(Json_DataType &json) {
    return json->get_member(JKEY_VERSION);
}

bool ICommand::apply_version(Json_DataType &json) {
    return json->set_member(JKEY_VERSION, std::string(VERSION) );
}

std::shared_ptr<ICommand::Twho> ICommand::extract_who(Json_DataType &json) {
    std::string app = Twho::STR_NULL;
    std::string pvd = Twho::STR_NULL;
    std::string func = Twho::STR_NULL;
    std::shared_ptr<Twho> result;
    assert(json.get() != NULL);

    try {
        // check validation.
        auto objects = json->get_member<Json_DataType>(JKEY_WHO);
        assert( objects.get() != NULL );
        app = objects->get_member(JKEY_WHO_APP);
        pvd = objects->get_member(JKEY_WHO_PVD);

        if( objects->has_member(JKEY_WHO_FUNC) == true )
            func = objects->get_member(JKEY_WHO_FUNC);

        result = std::make_shared<Twho>(app, pvd, func);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

bool ICommand::apply_who(Json_DataType &json, std::shared_ptr<Twho>& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(JKEY_WHO);
        assert( json_sub->set_member(JKEY_WHO_APP, value->get_app()) == true );
        assert( json_sub->set_member(JKEY_WHO_PVD, value->get_pvd()) == true );

        if( value->get_func() != Twho::STR_NULL ) {
            assert( json_sub->set_member(JKEY_WHO_FUNC, value->get_func()) == true );
        }

        json->set_member(JKEY_WHO, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::shared_ptr<ICommand::Twhen> ICommand::extract_when(Json_DataType &json, double def_time) {
    std::string type;
    double latency = Twhen::LATENCY_NULL;
    std::string week = Twhen::WEEK_NULL_STR;
    uint32_t period = Twhen::PERIOD_NULL;
    std::string date = Twhen::DATE_NULL_STR;
    std::string time = Twhen::TIME_NULL_STR;
    std::shared_ptr<Twhen> result;
    assert(json.get() != NULL);

    try {
        // check validation.
        auto objects = json->get_member<Json_DataType>(JKEY_WHEN);
        assert( objects.get() != NULL );
        type = objects->get_member(JKEY_WHEN_TYPE);

        // get value.
        auto sub_obj = objects->get_member<Json_DataType>(JKEY_WHEN_TIME);
        if( sub_obj->has_member(JKEY_WHEN_LATENCY) == true )
            latency = sub_obj->get_member<double>(JKEY_WHEN_LATENCY);

        if( sub_obj->has_member(JKEY_WHEN_WEEK) == true )
            week = sub_obj->get_member(JKEY_WHEN_WEEK);

        if( sub_obj->has_member(JKEY_WHEN_PERIOD) == true )
            period = sub_obj->get_member<uint32_t>(JKEY_WHEN_PERIOD);

        if( sub_obj->has_member(JKEY_WHEN_DATE) == true )
            date = sub_obj->get_member(JKEY_WHEN_DATE);

        if( sub_obj->has_member(JKEY_WHEN_TIME) == true )
            time = sub_obj->get_member(JKEY_WHEN_TIME);
        
        result = std::make_shared<Twhen>(type, date, time, week, period, latency, def_time);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

bool ICommand::apply_when(Json_DataType &json, std::shared_ptr<Twhen>& value) {
    Json_DataType json_sub;
    Json_DataType json_sub2;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(JKEY_WHEN);
        assert( json_sub->set_member(JKEY_WHEN_TYPE, value->get_type()) == true );

        json_sub2 = json_sub->set_member(JKEY_WHEN_TIME);
        if( value->get_latency() != Twhen::LATENCY_NULL ) {
            assert( json_sub2->set_member(JKEY_WHEN_LATENCY, value->get_latency()) == true );
        }
        else {
            if( value->get_time() != Twhen::TIME_NULL_STR ) {
                assert( json_sub2->set_member(JKEY_WHEN_DATE, value->get_date()) == true );
                assert( json_sub2->set_member(JKEY_WHEN_TIME, value->get_time()) == true );
            }

            if( value->get_week() != Twhen::WEEK_NULL_STR ) {
                assert( json_sub2->set_member(JKEY_WHEN_WEEK, value->get_week()) == true );
            }

            if( value->get_period() != Twhen::PERIOD_NULL ) {
                assert( json_sub2->set_member(JKEY_WHEN_PERIOD, value->get_period()) == true );
            }
        }

        json_sub->set_member(JKEY_WHEN_TIME, json_sub2.get());
        json->set_member(JKEY_WHEN, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::shared_ptr<ICommand::Twhere> ICommand::extract_where(Json_DataType &json) {
    std::string type;
    std::shared_ptr<Twhere> result;
    Json_DataType json_sub;
    assert(json.get() != NULL);

    try {
        // check validation.
        auto objects = json->get_member<Json_DataType>(JKEY_WHERE);
        assert( objects.get() != NULL );
        type = objects->get_member(JKEY_WHERE_TYPE);

        // get contents.
        json_sub = objects->get_member<Json_DataType>(JKEY_WHERE_CONTENTS);
        result = std::make_shared<Twhere>(type, json_sub);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

bool ICommand::apply_where(Json_DataType &json, std::shared_ptr<Twhere>& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(JKEY_WHERE);
        assert( json_sub->set_member(JKEY_WHERE_TYPE, value->get_type()) == true );
        json_sub->set_member(JKEY_WHERE_CONTENTS, value->encode().get());
        json->set_member(JKEY_WHERE, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::shared_ptr<ICommand::Twhat> ICommand::extract_what(Json_DataType &json) {
    std::string type;
    std::shared_ptr<Twhat> result;
    Json_DataType json_sub;
    assert(json.get() != NULL);

    try {
        // check validation.
        auto objects = json->get_member<Json_DataType>(JKEY_WHAT);
        assert( objects.get() != NULL );
        type = objects->get_member(JKEY_WHAT_TYPE);

        // get contents.
        json_sub = objects->get_member<Json_DataType>(JKEY_WHAT_CONTENTS);
        result = std::make_shared<Twhat>(type, json_sub);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

bool ICommand::apply_what(Json_DataType &json, std::shared_ptr<Twhat>& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(JKEY_WHAT);
        assert( json_sub->set_member(JKEY_WHAT_TYPE, value->get_type()) == true );
        json_sub->set_member(JKEY_WHAT_CONTENTS, value->encode().get());
        json->set_member(JKEY_WHAT, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::shared_ptr<ICommand::Thow> ICommand::extract_how(Json_DataType &json) {
    std::string type;
    std::shared_ptr<Thow> result;
    Json_DataType json_sub;
    assert(json.get() != NULL);

    try {
        // check validation.
        auto objects = json->get_member<Json_DataType>(JKEY_HOW);
        assert( objects.get() != NULL );
        type = objects->get_member(JKEY_HOW_TYPE);

        // get contents.
        json_sub = objects->get_member<Json_DataType>(JKEY_HOW_CONTENTS);
        result = std::make_shared<Thow>(type, json_sub);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

bool ICommand::apply_how(Json_DataType &json, std::shared_ptr<Thow>& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(JKEY_HOW);
        assert( json_sub->set_member(JKEY_HOW_TYPE, value->get_type()) == true );
        json_sub->set_member(JKEY_HOW_CONTENTS, value->encode().get());
        json->set_member(JKEY_HOW, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::shared_ptr<ICommand::Twhy> ICommand::extract_why(Json_DataType &json) {
    assert(json.get() != NULL);

    auto objects = json->get_member<Json_DataType>(JKEY_WHY);
    assert( objects.get() != NULL );
    return std::make_shared<Twhy>(objects->get_member(JKEY_WHY_DESP));
}

bool ICommand::apply_why(Json_DataType &json, std::shared_ptr<Twhy>& value) {
    Json_DataType json_sub;

    try {
        json_sub = json->set_member(JKEY_WHY);
        assert( json_sub->set_member(JKEY_WHY_DESP, *value) == true );
        json->set_member(JKEY_WHY, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}


/***********************************
 * Definition of Private Function.
 */
void ICommand::clear(void) {
    set_flag_parse(false);
    _payload_.clear();

    _who_.reset();
    _when_.reset();
    _where_.reset();
    _what_.reset();
    _how_.reset();
    _why_.reset();
}


} // namespace cmd
