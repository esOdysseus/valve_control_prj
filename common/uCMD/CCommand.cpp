#include <cassert>
#include <random>
#include <memory>
#include <iostream>
#include <string.h>

#include <logger.h>
#include <uCMD/CCommand.h>
#include <CException.h>
#include <IProtocolInf.h>
#include <Common.h>
#include <time_kes.h>


namespace cmd {

constexpr const char* CCommand::PROTOCOL_NAME;

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
static std::string JKEY_WHERE_GPS          = "gps";
static std::string JKEY_WHERE_GPS_LONG     = "long";
static std::string JKEY_WHERE_GPS_LAT      = "lat";
static std::string JKEY_WHAT               = "what";
static std::string JKEY_WHAT_TYPE          = "type";
static std::string JKEY_WHAT_SEQ           = "seq";
static std::string JKEY_HOW                = "how";
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


/**********************************
 * Definition of Public Function.
 */
CCommand::CCommand(std::string my_app_path, std::string my_pvd_id)
: _myself_from_( my_app_path, my_pvd_id ) {
    clear();
    assert(_myself_from_.empty() == false);
}

CCommand::CCommand( alias::CAlias& myself, FlagType flag_val)
: _myself_from_( myself ) {
    clear();

    _msg_id_ = gen_random_msg_id();
    _flag_ = flag_val;
    _state_ = 0;
    assert(_myself_from_.empty() == false);
}

// Copy Constructor
CCommand::CCommand( const CCommand& cmd )
: _myself_from_( cmd._myself_from_ ) {
    clear();

    // copy command
    _is_parsed_ = cmd._is_parsed_;
    _flag_ = cmd._flag_;
    _state_ = cmd._state_;
    _msg_id_ = cmd._msg_id_;
    _send_time_d_ = cmd._send_time_d_;
    _rcv_time_ = cmd._rcv_time_;
    
    _who_ = cmd._who_;
    _when_ = cmd._when_;
    _where_ = cmd._where_;
    _what_ = cmd._what_;
    _how_ = cmd._how_;
    _why_ = cmd._why_;
}

CCommand::~CCommand(void) {
    clear();
}

void CCommand::clear(void) {
    _msg_id_ = 0;
    _flag_ = E_FLAG::E_FLAG_NONE;
    _state_ = 0;
    _send_time_d_ = 0.0;
    _rcv_time_ = 0.0;

    _who_.reset();
    _when_.reset();
    _where_.reset();
    _what_.reset();
    _how_.reset();
    _why_.reset();
    _is_parsed_ = false;
}

// presentator
bool CCommand::decode(std::shared_ptr<IProtocolInf>& protocol) {
    size_t payload_size;
    struct timespec temp;

    if( protocol.get() == NULL ) {
        LOGW("Protocol is empty.");
        return false;
    }

    try {
        std::string from_full_path;
        Json_DataType json_manager;
        const char* payload = (const char*)protocol->get_payload(payload_size);

        if( payload == NULL || payload_size <= 0 ) {
            LOGERR("Payload(0x%X) is NULL or length(%u) <= 0.", payload, payload_size);
            throw std::invalid_argument("Payload is NULL or length <= 0.");
        }

        if( _is_parsed_ == false) {
            // unpacking
            _flag_ = std::stoi(protocol->get_property("flag"), nullptr, 10);
            _state_ = std::stoi(protocol->get_property("state"), nullptr, 10);
            _msg_id_ = std::stoi(protocol->get_property("msg_id"), nullptr, 10);
            from_full_path = protocol->get_property("from");
            // Don't need to convert from_full_path to app_path & pvd_id.

            // parsing when time.
            _send_time_d_ = std::stod(protocol->get_property("when"));

            // parsing json payload (where, what, how, why)
            json_manager = std::make_shared<json_mng::CMjson>();
            LOGD("strlen(payload)=%d , length=%d", strlen(payload), payload_size);
            assert( json_manager->parse(payload, payload_size) == true);
            LOGD( "Success parse of Json buffer." );
            // _who_ = extract_who(json_manager);
            // _when_ = extract_when(json_manager);
            // _where_ = extract_where(json_manager);
            _what_ = extract_what(json_manager);
            _how_ = extract_how(json_manager);
            _why_ = extract_why(json_manager);

            // mark receive-time of this packet using my-system time.
            assert( (_rcv_time_=time_pkg::CTime::get<double>()) > 0.0 );
            _is_parsed_ = true;
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw CException(E_ERROR::E_ERR_FAIL_DECODING_CMD);
    }

    return _is_parsed_;
}

std::shared_ptr<payload::CPayload> CCommand::encode( std::shared_ptr<ICommunicator>& handler ) {
    const char* body = NULL;
    std::shared_ptr<payload::CPayload> message;
    std::shared_ptr<IProtocolInf> protocol;

    if( handler.get() == NULL ) {
        LOGW("Communicator is not exist.");
        return message;
    }

    try {
        Json_DataType json_manager;

        message = handler->create_payload();
        if( message.get() == NULL ) {
            throw std::logic_error("Message-Creating is failed.");
        }
        protocol = message->get(PROTOCOL_NAME);
    
        protocol->set_property("flag", _flag_);
        protocol->set_property("state", _state_);
        protocol->set_property("msg_id", _msg_id_);

        if ( get_flag(E_FLAG_ACK_MSG) == false && 
             get_flag(E_FLAG_ACTION_DONE) == false && 
             get_flag(E_FLAG_KEEPALIVE) == false ) {

            // make json body (where, what, how, why)
            json_manager = std::make_shared<json_mng::CMjson>();
            assert( json_manager.get() != NULL );
            // assert(apply_who(json_manager, _who_) == true);
            // assert(apply_when(json_manager, _when_) == true);
            // assert(apply_where(json_manager, _where_) == true);
            assert(apply_what(json_manager, _what_) == true);
            assert(apply_how(json_manager, _how_) == true);
            assert(apply_why(json_manager, _why_) == true);
            assert( (body = json_manager->print_buf()) != NULL );

            protocol->set_payload( body, strlen(body) );
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        message.reset();
        protocol.reset();
        throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
    }

    return message;
}

// getter
CCommand::FlagType CCommand::get_flag(FlagType pos) {
    assert( pos != (FlagType)E_FLAG::E_FLAG_NONE);
    return _flag_ & pos;
}

// setter
void CCommand::set_id(unsigned long value) { 
    _msg_id_ = value;
    if( _msg_id_ == 0 ) {
        _msg_id_ = gen_random_msg_id();
    }
}

void CCommand::set_flag(E_FLAG pos, FlagType value) {
    int shift_cnt = 0;

    if ( pos == E_FLAG::E_FLAG_NONE ) {
        _flag_ = value;
    }
    else {
        // Assumption : pos is continuous-bitmask.
        while( ((1<<shift_cnt) & pos) == 0 ) {
            shift_cnt++;
            assert( shift_cnt < (sizeof(FlagType)*8) );
        }

        _flag_ = (_flag_ & (~pos)) | ((value << shift_cnt) & pos);
    }
}

void CCommand::set_state(uint16_t value) {
    _state_ = value;
}

void CCommand::set_when( std::string type, double start_time, 
                                           Twhen::TEweek week, 
                                           uint32_t period, 
                                           double latency ) {
    _when_.reset();
    _when_ = std::make_shared<Twhen>(type, start_time, week, period, latency);
}

void CCommand::set_how( std::string method, std::string post_method, double costtime ) {
    _how_.reset();
    _how_ = std::make_shared<Thow>(method, post_method, costtime);
}


// printer
std::string CCommand::print_send_time(void) {  // print when-data for human-readable.
    assert( parsing_complet() == true );
    char when_string[100];

    try {
        auto time_tm = time_pkg::CTime::convert<struct tm>( _send_time_d_ );
        strftime(when_string, 100, "CMD-When is [%B %d, %Y] time is [%T]", &time_tm);
        LOGD( "%s", when_string );
        return when_string;
    }
    catch( const std::exception &e ) {
        LOGERR("print_send_time is failed." );
    }

    return std::string();
}

// compare time
E_CMPTIME CCommand::compare_with_curtime(double duty) {   // check whether current-time is over/under/equal with cmd-time.
    try {
        if (  parsing_complet() == false ) {
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

E_CMPTIME CCommand::compare_with_another(CCommand *cmd, double duty) {
    assert( cmd != NULL );
    try {
        if (  parsing_complet() == false ) {
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

CCommand::Twho& CCommand::who(void) { 
    if( parsing_complet() == false ) {
        throw std::out_of_range("Twho Parsing is not complete.");
    }
    assert( _who_.get() != NULL );
    return *_who_; 
}

CCommand::Twhen& CCommand::when(void) { 
    if( parsing_complet() == false ) {
        throw std::out_of_range("Twhen Parsing is not complete.");
    }
    assert( _when_.get() != NULL );
    return *_when_; 
}

CCommand::Twhere& CCommand::where(void) { 
    if( parsing_complet() == false ) {
        throw std::out_of_range("Twhere Parsing is not complete.");
    }
    assert( _where_.get() != NULL );
    return *_where_; 
}

CCommand::Twhat& CCommand::what(void) { 
    if( parsing_complet() == false ) {
        throw std::out_of_range("Twhat Parsing is not complete.");
    }
    assert( _what_.get() != NULL );
    return *_what_; 
}

CCommand::Thow& CCommand::how(void) { 
    if( parsing_complet() == false ) {
        throw std::out_of_range("Thow Parsing is not complete.");
    }
    assert( _how_.get() != NULL );
    return *_how_; 
}

CCommand::Twhy& CCommand::why(void) { 
    if( parsing_complet() == false ) {
        throw std::out_of_range("Twhy Parsing is not complete.");
    }
    assert( _why_.get() != NULL );
    return *_why_; 
}


/***********************************
 * Definition of Private Function.
 */
std::shared_ptr<CCommand::Twho> CCommand::extract_who(Json_DataType &json) {
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

std::shared_ptr<CCommand::Twhen> CCommand::extract_when(Json_DataType &json) {
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

        // get valve value.
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
        
        result = std::make_shared<Twhen>(type, date, time, week, period, latency);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

std::shared_ptr<CCommand::Twhere> CCommand::extract_where(Json_DataType &json) {
    std::string type;
    double longitude = Twhere::GPS_NULL;
    double latitude = Twhere::GPS_NULL;
    std::shared_ptr<Twhere> result;
    assert(json.get() != NULL);

    try {
        // check validation.
        auto objects = json->get_member<Json_DataType>(JKEY_WHERE);
        assert( objects.get() != NULL );
        type = objects->get_member(JKEY_WHERE_TYPE);

        // get valve value.
        auto sub_obj = objects->get_member<Json_DataType>(JKEY_WHERE_GPS);
        longitude = sub_obj->get_member<double>(JKEY_WHERE_GPS_LONG);
        latitude = sub_obj->get_member<double>(JKEY_WHERE_GPS_LAT);

        result = std::make_shared<Twhere>(type, longitude, latitude);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

std::shared_ptr<CCommand::Twhat> CCommand::extract_what(Json_DataType &json) {
    std::string type;
    int seq_num = -1;
    assert(json.get() != NULL);

    // check validation.
    auto objects = json->get_member<Json_DataType>(JKEY_WHAT);
    assert( objects.get() != NULL );
    type = objects->get_member(JKEY_WHAT_TYPE);

    // get valve value.
    seq_num = objects->get_member<int>(JKEY_WHAT_SEQ);
    return std::make_shared<Twhat>(type, seq_num);
}

bool CCommand::apply_what(Json_DataType &json, std::shared_ptr<Twhat>& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(JKEY_WHAT);
        assert( json_sub->set_member(JKEY_WHAT_TYPE, value->get_type()) == true );
        assert( json_sub->set_member(JKEY_WHAT_SEQ, value->get_which()) == true );
        json->set_member(JKEY_WHAT, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::shared_ptr<CCommand::Thow> CCommand::extract_how(Json_DataType &json) {
    assert(json.get() != NULL);

    // get method
    auto objects = json->get_member<Json_DataType>(JKEY_HOW);
    assert( objects.get() != NULL );
    auto method = objects->get_member(JKEY_HOW_METHOD);
    auto method_post = objects->get_member(JKEY_HOW_METHOD_POST);
    
    // get costtime
    auto cost_time = objects->get_member(JKEY_HOW_COSTTIME);
    return std::make_shared<Thow>(method, method_post, cost_time);
}

bool CCommand::apply_how(Json_DataType &json, std::shared_ptr<Thow>& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(JKEY_HOW);
        assert( json_sub->set_member(JKEY_HOW_METHOD, value->get_method()) == true );
        assert( json_sub->set_member(JKEY_HOW_METHOD_POST, value->get_post_method()) == true );
        assert( json_sub->set_member(JKEY_HOW_COSTTIME, value->get_costtime()) == true );
        json->set_member(JKEY_HOW, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::shared_ptr<CCommand::Twhy> CCommand::extract_why(Json_DataType &json) {
    assert(json.get() != NULL);

    auto objects = json->get_member<Json_DataType>(JKEY_WHY);
    assert( objects.get() != NULL );
    return std::make_shared<Twhy>(objects->get_member(JKEY_WHY_DESP));
}

bool CCommand::apply_why(Json_DataType &json, std::shared_ptr<Twhy>& value) {
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

uint32_t CCommand::gen_random_msg_id(void) {
    uint32_t new_msgid = 0;
    int32_t id_min = 1;
    int32_t id_max = 2147483647;
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen( rd() ); 
    std::uniform_int_distribution<> dist(id_min, id_max); 
    
    LOGD("Random MSG-ID Min : %d", (uint32_t)(dist.min()) );
    LOGD("Random MSG-ID Max : %d", (uint32_t)(dist.max()) );

    new_msgid = (uint32_t)(dist( gen ));
    assert( ((uint32_t)id_min) <= new_msgid && new_msgid <= ((uint32_t)id_max) );
    LOGI("Generated new MSG-ID=%d", new_msgid);

    return new_msgid;
}


} // namespace cmd
