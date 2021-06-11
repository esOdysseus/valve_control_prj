#include <cassert>
#include <random>
#include <memory>
#include <iostream>
#include <string.h>

#include <logger.h>
#include <CCommand.h>
#include <CException.h>
#include <IProtocolInf.h>
#include <Common.h>
#include <time_kes.h>


namespace valve_pkg {

#define BODY_KEY_WHAT           "what"
#define BODY_KEY_WHAT_TYPE      "type"
#define BODY_KEY_WHAT_SEQ       "seq"
#define BODY_KEY_HOW            "how"
#define BODY_KEY_HOW_METHOD     "method"
#define BODY_KEY_HOW_COSTTIME   "costtime"
#define BODY_KEY_WHY            "why"
#define BODY_KEY_WHY_DESP       "desp"
#define BODY_KEY_WHY_OBJ        "objective"
#define BODY_KEY_WHY_DEP        "dependency"

#define BODY_VAL_WHAT_TYPE      "valve-swc"

/***********************************
 * Example Payload
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

class CCommand::CVwhat {
public:
    CVwhat( std::string type, int which ) {
        _type_ = type;

        switch(which) {
        case 0:
            _which_ = E_TARGET::E_VALVE_LEFT_01;
            break;
        case 1:
            _which_ = E_TARGET::E_VALVE_LEFT_02;
            break;
        case 2:
            _which_ = E_TARGET::E_VALVE_LEFT_03;
            break;
        case 3:
            _which_ = E_TARGET::E_VALVE_LEFT_04;
            break;
        default:
            LOGERR("Invalid BODY_KEY_WHAT_SEQ(%d) data.", which );
            throw CException(E_ERROR::E_ERR_INVALID_VALUE);
        }
        validation_check();
    }

    ~CVwhat( void ) {
        _type_.clear();
        _which_ = E_TARGET::E_VALVE_NULL;
    }

    std::string get_type(void) {    return _type_;  }

    E_TARGET get_which(void) { return _which_; }

private:
    CVwhat(void) = delete;

    void validation_check(void) {
        if( E_TARGET::E_VALVE_LEFT_01 > _which_ || _which_ > E_TARGET::E_VALVE_LEFT_04 ) {
            throw std::invalid_argument("CVwhat whick is invalid-data. value is out-of-range.");
        }
    }

private:
    std::string _type_;     // valid : 'valve-swc'
    E_TARGET _which_;       // valid-range: 0 ~ 3
};


class CCommand::CVhow {
public:
    CVhow( std::string method, std::string cost_time ) {
        _method_ = method;
        _cost_time_ = std::stod( cost_time, nullptr );
        if ( _method_ == OPEN ) {
            assert( _cost_time_ > 1.0 );
        }
    }

    ~CVhow( void ) {
        _method_.clear();
        _cost_time_ = 0.0;
    }

    std::string get_method(void) {    return _method_;  }

    double get_costtime(void) { return _cost_time_; }

private:
    CVhow(void) = delete;

private:
    std::string _method_;     // valid : 'open', 'close'
    double _cost_time_;       // desp: if method is 'open' then it is cost-time for close the opened-value.
};



/**********************************
 * Definition of Public Function.
 */
CCommand::CCommand(std::string my_app_path, std::string my_pvd_id)
: _myself_from_( my_app_path, my_pvd_id ) {
    clear();
    assert(_myself_from_.empty() == false);
}

CCommand::CCommand( alias::CAlias& myself, const CCommand *cmd, FlagType flag_val)
: _myself_from_( myself ) {
    assert( cmd != NULL );
    clear();

    // clone
    _msg_id_ = cmd->_msg_id_;
    _flag_ = flag_val;
    _state_ = 0;
    assert(_myself_from_.empty() == false);
    _what_ = cmd->_what_;
    _how_ = cmd->_how_;
    _why_ = cmd->_why_;

    if ( get_flag(E_FLAG::E_FLAG_ACK_MSG) || get_flag(E_FLAG::E_FLAG_KEEPALIVE) ) {
        assert( set_when() == true );
    }
    else {
        _when_tm_ = cmd->_when_tm_;
        _when_double_ = cmd->_when_double_;
    }
}

CCommand::CCommand( alias::CAlias& myself, FlagType flag_val)
: _myself_from_( myself ) {
    clear();

    _msg_id_ = gen_random_msg_id();
    _flag_ = flag_val;
    _state_ = 0;
    assert(_myself_from_.empty() == false);
    // _what_ = E_TARGET::E_VALVE_NULL;

    if ( get_flag(E_FLAG::E_FLAG_ACK_MSG) || get_flag(E_FLAG::E_FLAG_KEEPALIVE) ) {
        assert( set_when() == true );
    }
}

CCommand::~CCommand(void) {
    clear();
}

void CCommand::clear(void) {
    _msg_id_ = 0;
    _flag_ = E_FLAG::E_FLAG_NONE;
    _state_ = 0;
    bzero((void*)&_when_tm_, sizeof(_when_tm_));
    _when_double_ = 0.0;
    _rcv_time_ = 0.0;
    _what_.reset();
    _how_.reset();
    _why_.clear();
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
            // std::string err = "Payload(0x" + std::to_string( reinterpret_cast<int>(payload) ) + ") is Null or length(" + std::to_string(payload_size) + ") <= 0.";
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
            _when_double_ = std::stod(protocol->get_property("when"));
            assert( apply_double2utc(_when_double_, temp) == true );
            /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */
            localtime_r((time_t *)&temp.tv_sec, &_when_tm_); 

            // parsing json payload (where, what, how, why)
            json_manager = std::make_shared<json_mng::CMjson>();
            LOGD("strlen(payload)=%d , length=%d", strlen(payload), payload_size);
            assert( json_manager->parse(payload, payload_size) == true);
            LOGD( "Success parse of Json buffer." );
            _what_ = extract_what(json_manager);
            _how_ = extract_how(json_manager);
            _why_ = extract_why(json_manager);

            // mark receive-time of this packet using my-system time.
            assert( (_rcv_time_=get_cur_time()) > 0.0 );
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

// setter
bool CCommand::set_when(void) {    // set current-time to command.
    struct timespec t_now;

    try {
        // get current-time by REAL-TIME-CLOCK.
        assert( clock_gettime(CLOCK_REALTIME, &t_now) != -1 );

        /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */
        localtime_r((time_t *)&t_now.tv_sec, &_when_tm_); 
        assert( (_when_double_ = extract_utc2double(t_now)) > 0.0 );
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("set_when0 is failed." );
    }

    return false;
}

bool CCommand::set_when(E_WEEK next) {  // set next week-day base on cmd-time. (hour/minute/sec is zero-set.)
    struct timespec temp;
    int diff = ((next + 7) - get_week(_when_tm_)) % 7;

    try{
        if (diff == 0) {
            diff = 7;
        }

        if (diff > 0 ) {
            _when_double_ += (double)(diff * 24 * 3600);
            assert( apply_double2utc(_when_double_, temp) == true );
            /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */
            localtime_r((time_t *)&temp.tv_sec, &_when_tm_); 
        }
        assert(next == get_week(_when_tm_));
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("set_when1 is failed." );
    }

    return false;
}

bool CCommand::set_when(int hour, int minute, int seconds) {   // set time base on cmd-time.
    assert( hour >= 0 );
    assert( minute >= 0 );
    assert( seconds >= 0 );

    try {
        _when_tm_.tm_hour = hour;
        _when_tm_.tm_min = minute;
        _when_tm_.tm_sec = seconds;
        _when_double_ = (double)(mktime(&_when_tm_)) + (_when_double_ - (double)((long)_when_double_));
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("set_when2 is failed." );
    }

    return false;
}

bool CCommand::set_when(int year, int month, int day, int hour, int minute, int seconds) {   // set next time base on current-time.
    assert( year >= 0 );
    assert( month >= 0 );
    assert( day >= 0 );
    assert( hour >= 0 );
    assert( minute >= 0 );
    assert( seconds >= 0 );

    try {
        _when_tm_.tm_year = year;
        _when_tm_.tm_mon = month;
        _when_tm_.tm_mday = day;
        _when_tm_.tm_hour = hour;
        _when_tm_.tm_min = minute;
        _when_tm_.tm_sec = seconds;
        _when_double_ = (double)(mktime(&_when_tm_)) + (_when_double_ - (double)((long)_when_double_));
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("set_when3 is failed." );
    }

    return false;
}

bool CCommand::append_when(int hour, int minute, int seconds) {
    struct timespec temp;
    assert( hour >= 0 );
    assert( minute >= 0 );
    assert( seconds >= 0 );

    try {
        // sum as seconds.
        seconds = seconds + (minute * 60) + (hour * 3600);
        
        // update double seconds
        _when_double_ += (double)seconds;
        assert( apply_double2utc(_when_double_, temp) == true );
        /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */
        localtime_r((time_t *)&temp.tv_sec, &_when_tm_); 
        return true;
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
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

CCommand::FlagType CCommand::get_flag(FlagType pos) {
    assert( pos != (FlagType)E_FLAG::E_FLAG_NONE);
    // assert( parsing_complet() == true );
    return _flag_ & pos;
}

uint16_t CCommand::get_state(void) {
    return _state_;
}

template <>
double CCommand::get_when<double>(void) { 
    return _when_double_; 
}

template <>
struct timespec CCommand::get_when<struct timespec>(void) { 
    struct timespec res;
    assert( apply_double2utc(_when_double_, res)==true );
    return res; 
}

double CCommand::get_cur_time(void) {
    struct timespec t_now;
    try {
        // get current-time by REAL-TIME-CLOCK.
        assert( clock_gettime(CLOCK_REALTIME, &t_now) != -1 );
        return extract_utc2double(t_now);
    }
    catch( const std::exception &e ) {
        LOGERR("get_cur_time is failed." );
    }

    return -1.0;
}

CCommand::E_TARGET CCommand::get_what_valve(void) {
    if( _what_.get() != NULL ) {
        return _what_->get_which();
    }
    return CCommand::E_TARGET::E_VALVE_NULL;
}

std::string CCommand::get_how_method(void) {
    std::string res;

    if( _how_.get() != NULL ) {
        res = _how_->get_method();
    }
    return res;
}

double CCommand::get_how_costtime(void) {
    if( _how_.get() != NULL ) {
        return _how_->get_costtime();
    }
    return 0.0;
}

// printer
std::string CCommand::print_when(void) {  // print when-data for human-readable.
    assert( parsing_complet() == true );
    char when_string[100];

    try {
        strftime(when_string, 100, "CMD-When is [%B %d, %Y] time is [%T]", &_when_tm_);
        LOGD( "%s", when_string );
        return when_string;
    }
    catch( const std::exception &e ) {
        LOGERR("print_when is failed." );
    }

    return std::string();
}

std::string CCommand::print_cur_time(void) {  // print current-time for human-readable.
    struct timespec tspec;
    tm cur_tm;
    char cur_string[100];

    try {
        // get current-time by REAL-TIME-CLOCK.
        if (clock_gettime(CLOCK_REALTIME, &tspec) == -1) {  
            LOGERR("clock_gettime() is failed." );
            return std::string();
        }

        /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */
        localtime_r((time_t *)&tspec.tv_sec, &cur_tm); 

        strftime(cur_string, 100, "Today is [%B %d, %Y] time is [%T]", &cur_tm);
        LOGD( "%s", cur_string );
        return cur_string;
    }
    catch( const std::exception &e ) {
        LOGERR("print_cur_time is failed." );
    }

    return std::string();
}

// compare time
E_CMPTIME CCommand::check_with_curtime(double duty) {   // check whether current-time is over/under/equal with cmd-time.
    assert( parsing_complet() == true );
    struct timespec t_now;
    double d_now = 0.0;

    // get current-time by REAL-TIME-CLOCK.
    assert( clock_gettime(CLOCK_REALTIME, &t_now) != -1 );  
    assert( (d_now = extract_utc2double(t_now)) > 0.0 );

    if ( _when_double_ < (d_now - duty) ) {
        return E_CMPTIME::E_CMPTIME_UNDER;
    }
    else if( _when_double_ > (d_now + duty) ) {
        return E_CMPTIME::E_CMPTIME_OVER;
    }
    else {
        return E_CMPTIME::E_CMPTIME_EQUAL;
    }

    return E_CMPTIME::E_CMPTIME_UNKNOWN;
}

E_CMPTIME CCommand::check_with_another(CCommand *cmd, double duty) {
    assert( cmd != NULL );
    assert( parsing_complet() == true );
    double d_another = cmd->get_when();

    if ( _when_double_ < (d_another - duty) ) {
        return E_CMPTIME::E_CMPTIME_UNDER;
    }
    else if( _when_double_ > (d_another + duty) ) {
        return E_CMPTIME::E_CMPTIME_OVER;
    }
    else {
        return E_CMPTIME::E_CMPTIME_EQUAL;
    }

    return E_CMPTIME::E_CMPTIME_UNKNOWN;
}



/***********************************
 * Definition of Private Function.
 */
E_WEEK CCommand::get_week(struct tm &time) {
    return (E_WEEK)time.tm_wday;
}

double CCommand::extract_utc2double(struct timespec &time) {
    double res = 0.0;

    res += (double)time.tv_sec;
    res += (double)time.tv_nsec / 1000000000.0;

    LOGD( "extracted UTC time = %f", res );
    return res;
}

bool CCommand::apply_double2utc(double &time, struct timespec &target) {
    try {
        target.tv_sec = (long)time;
        target.tv_nsec = (time - (long)time) * 1000000000L;
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("apply_double2utc is failed." );
    }
    return false;
}

CCommand::Twhat CCommand::extract_what(Json_DataType &json) {
    std::string type;
    int seq_num = -1;
    assert(json.get() != NULL);

    // check validation.
    auto objects = json->get_member<Json_DataType>(BODY_KEY_WHAT);
    assert( objects.get() != NULL );
    type = objects->get_member(BODY_KEY_WHAT_TYPE);
    assert( type == BODY_VAL_WHAT_TYPE );

    // get valve value.
    seq_num = objects->get_member<int>(BODY_KEY_WHAT_SEQ);
    return std::make_shared<CVwhat>(type, seq_num);
}

bool CCommand::apply_what(Json_DataType &json, Twhat& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(BODY_KEY_WHAT);
        assert( json_sub->set_member(BODY_KEY_WHAT_TYPE, BODY_VAL_WHAT_TYPE) == true );
        assert( json_sub->set_member(BODY_KEY_WHAT_SEQ, (int)value->get_which()) == true );
        json->set_member(BODY_KEY_WHAT, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

CCommand::Thow CCommand::extract_how(Json_DataType &json) {
    assert(json.get() != NULL);

    // get method
    auto objects = json->get_member<Json_DataType>(BODY_KEY_HOW);
    assert( objects.get() != NULL );
    auto method = objects->get_member(BODY_KEY_HOW_METHOD);
    // get costtime
    auto cost_time = objects->get_member(BODY_KEY_HOW_COSTTIME);
    return std::make_shared<CVhow>(method.data(), cost_time.data());
}

bool CCommand::apply_how(Json_DataType &json, Thow& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(BODY_KEY_HOW);
        assert( json_sub->set_member(BODY_KEY_HOW_METHOD, value->get_method()) == true );
        assert( json_sub->set_member(BODY_KEY_HOW_COSTTIME, value->get_costtime()) == true );
        json->set_member(BODY_KEY_HOW, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

CCommand::Twhy CCommand::extract_why(Json_DataType &json) {
    assert(json.get() != NULL);

    auto objects = json->get_member<Json_DataType>(BODY_KEY_WHY);
    assert( objects.get() != NULL );
    return objects->get_member(BODY_KEY_WHY_DESP);
}

bool CCommand::apply_why(Json_DataType &json, Twhy& value) {
    Json_DataType json_sub;

    try {
        json_sub = json->set_member(BODY_KEY_WHY);
        assert( json_sub->set_member(BODY_KEY_WHY_DESP, value) == true );
        json->set_member(BODY_KEY_WHY, json_sub.get());
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


} // namespace valve_pkg
