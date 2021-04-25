#include <cassert>
#include <random>
#include <memory>
#include <iostream>
#include <string.h>

#include <logger.h>
#include <CVctrlCMD.h>
#include <CException.h>


namespace valve_pkg {

#define BODY_KEY_WHAT           "what"
#define BODY_KEY_WHAT_TYPE      "type"
#define BODY_KEY_WHAT_SEQ       "seq"
#define BODY_KEY_HOW            "how"
#define BODY_KEY_HOW_METHOD     "method"
#define BODY_KEY_WHY            "why"
#define BODY_KEY_WHY_DESP       "desp"
#define BODY_KEY_WHY_OBJ        "objective"
#define BODY_KEY_WHY_DEP        "dependency"

#define BODY_VAL_WHAT_TYPE      "valve-swc"

/**********************************
 * Definition of Public Function.
 */
CVctrlCMD::CVctrlCMD(std::string myself_name) {
    clear();
    from = myself_name;
    assert(from.empty() == false);
}

CVctrlCMD::CVctrlCMD(std::string myself_name, 
                     const CVctrlCMD *cmd, 
                     std::string target, 
                     FlagType flag_val) {
    struct timespec temp;
    assert( cmd != NULL );
    clear();

    // clone
    msg_id = cmd->msg_id;
    _flag_ = flag_val;
    _state_ = 0;
    from = myself_name;
    assert(from.empty() == false);
    who = target;
    what = cmd->what;
    how = cmd->how;
    why = cmd->why;

    if ( get_flag(E_FLAG::E_FLAG_ACK_MSG) || get_flag(E_FLAG::E_FLAG_KEEPALIVE) ) {
        assert( set_when() == true );
    }
    else {
        when_tm = cmd->when_tm;
        when_double = cmd->when_double;
    }
}

CVctrlCMD::CVctrlCMD(std::string myself_name,
                     std::string target_app,
                     FlagType flag_val) {

    // LOGD("*********** header *************");
    // LOGD("* header : start=%X , size=%d", &_header_, sizeof(_header_));
    // LOGD("* sof    : start=%X , size=%d", &(_header_.sof), sizeof(_header_.sof));
    // LOGD("* from   : start=%X , size=%d", &(_header_.from), sizeof(_header_.from));
    // LOGD("* who    : start=%X , size=%d", &(_header_.who), sizeof(_header_.who));
    // LOGD("* when   : start=%X , size=%d", &(_header_.when), sizeof(_header_.when));
    // LOGD("* tv_sec : start=%X , size=%d", &(_header_.when.tv_sec), sizeof(_header_.when.tv_sec));
    // LOGD("* tv_nsec: start=%X , size=%d", &(_header_.when.tv_nsec), sizeof(_header_.when.tv_nsec));
    // LOGD("* length : start=%X , size=%d", &(_header_.length), sizeof(_header_.length));
    // LOGD("");

    clear();

    msg_id = gen_random_msg_id();
    _flag_ = flag_val;
    _state_ = 0;
    from = myself_name;
    assert(from.empty() == false);
    who = target_app;
    what = E_TARGET::E_VALVE_NULL;

    if ( get_flag(E_FLAG::E_FLAG_ACK_MSG) || get_flag(E_FLAG::E_FLAG_KEEPALIVE) ) {
        assert( set_when() == true );
    }
}

CVctrlCMD::~CVctrlCMD(void) {
    clear();
}

void CVctrlCMD::clear(void) {
    bzero((void*)&_header_, sizeof(_header_));
    msg_id = 0;
    _flag_ = E_FLAG::E_FLAG_NONE;
    _state_ = 0;
    from.clear();
    who.clear();
    bzero((void*)&when_tm, sizeof(when_tm));
    when_double = 0.0;
    _rcv_time_ = 0.0;
    what = E_TARGET::E_VALVE_NULL;
    how.clear();
    why.clear();
    is_parsed = false;
}

bool CVctrlCMD::check_sof_for_decode(const void *payload, ssize_t payload_size) {
    std::string sof;

    if ( payload_size < sizeof(_header_) ) {
        LOGW("payload size is invalid.(%d) We expect-size is over %d.", payload_size, sizeof(_header_));
        return false;
    }

    // SOF validation check.
    sof = get_sof(payload, payload_size);
    if ( sof != SOF_ ) {
        LOGW("SOF value is invalid.(%s)", sof.c_str());
        return false;
    }

    return true;
}

// presentator
bool CVctrlCMD::decode(const void * payload, ssize_t payload_size) {
    char* body = NULL;
    struct timespec when_dumy;
    Json_DataType json_manager;

    assert( payload != NULL);
    assert( payload_size > 0 );
    LOGD( "Total-Size=%d, header-size=%d", payload_size, sizeof(_header_) );

    if( is_parsed == false) {
        try {
            // SOF field check.
            if ( check_sof_for_decode(payload, payload_size) == false ) {
                LOGW("This packet is not UCMD packet.");
                return false;
            }

            // unpacking
            memcpy ((void*)&_header_, payload, sizeof(_header_));
            assert( payload_size == sizeof(_header_) + _header_.length );
            if ( _header_.length > 0 ) {
                assert( (body = new char[_header_.length]) != NULL );
                bzero(body, _header_.length);
                memcpy(body, payload + sizeof(_header_), sizeof(char)*_header_.length );
                LOGD("%s", body);
            }

            // parsing (flag, state, msg-id, from, who, when)
            _flag_ = _header_.flag;
            _state_ = _header_.state;
            msg_id = _header_.MSG_ID;
            if( from.empty() == true ) {
                from = _header_.from;
            }
            who = _header_.who;
            localtime_r((time_t *)&(_header_.when.tv_sec), &when_tm); 
            when_dumy.tv_sec = _header_.when.tv_sec;
            when_dumy.tv_nsec= _header_.when.tv_nsec;
            assert( (when_double = extract_utc2double(when_dumy)) > 0 );

            if ( body != NULL ) {
                // parsing json body (where, what, how, why)
                json_manager = std::make_shared<json_mng::CMjson>();
                LOGD("strlen(body)=%d , length=%d", strlen(body), _header_.length-1);
                assert( json_manager->parse(body, _header_.length-1) == true);
                LOGD( "Success parse of Json buffer." );
                what = extract_what(json_manager);
                how = extract_how(json_manager);
                why = extract_why(json_manager);
            }

            // mark receive-time of this packet using my-system time.
            assert( (_rcv_time_=get_cur_time()) > 0.0 );

            is_parsed = true;
        }
        catch (const std::exception &e) {
            LOGERR( "%s", e.what() );
            if (body != NULL) {
                delete[] body;
                body = NULL;
            }
            throw CException(E_ERROR::E_ERR_FAIL_DECODING_CMD);
        }
    }

    // free memory
    if (body != NULL) {
        delete[] body;
        body = NULL;
    }

    return is_parsed;
}

void * CVctrlCMD::encode(ssize_t &size) {
    const char* body = NULL;
    void* raw_packet = NULL;
    struct timespec when_dumy;
    Json_DataType json_manager;
    ssize_t leng = WHO_CAP;
    size = 0;

    try {
        if ( get_flag(E_FLAG_ACK_MSG) == false && 
             get_flag(E_FLAG_ACTION_DONE) == false && 
             get_flag(E_FLAG_KEEPALIVE) == false ) {

            // make json body (where, what, how, why)
            json_manager = std::make_shared<json_mng::CMjson>();
            assert( json_manager.get() != NULL );
            assert(apply_what(json_manager, what) == true);
            assert(apply_how(json_manager, how) == true);
            assert(apply_why(json_manager, why) == true);
            assert( (body = json_manager->print_buf()) != NULL );
        }

        // make _header_ (flag, state, msg-id, from, who, when, body-length)
        bzero((void*)&_header_, sizeof(_header_));
        memcpy( _header_.sof, (void*)&SOF_, 4);
        _header_.flag = _flag_;
        _header_.state = _state_;
        _header_.MSG_ID = msg_id;
        // write from
        leng = WHO_CAP;
        if( strlen(from.c_str()) < WHO_CAP ) {
            leng = strlen(from.c_str());
        }
        strncpy(_header_.from, from.c_str(), leng);
        // write who
        leng = WHO_CAP;
        if( strlen(who.c_str()) < WHO_CAP ) {
            leng = strlen(who.c_str());
        }
        strncpy(_header_.who, who.c_str(), leng);

        assert( apply_double2utc(when_double, when_dumy) == true );
        _header_.when.tv_sec = when_dumy.tv_sec;
        _header_.when.tv_nsec = when_dumy.tv_nsec;
        _header_.length = 0;
        if ( body != NULL ) {
            _header_.length = strlen(body)+1;
            LOGD("_header_.length=%d", _header_.length);
        }

        // packing
        assert( (raw_packet = (void*)(new char[_header_.length + sizeof(_header_)])) != NULL );
        memcpy(raw_packet, (void*)&_header_, sizeof(_header_));
        if ( body != NULL ) {
            memcpy(raw_packet + sizeof(_header_), body, _header_.length);
        }
        size = sizeof(_header_) + _header_.length;
        LOGD("Total-Size=%d, header-size=%d, body.length=%d",size, sizeof(_header_), _header_.length);
        if( _header_.length > 0 ) {
            LOGD( "body check: [%s]", (char*)raw_packet + sizeof(_header_) );
        }

        return raw_packet;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        if (raw_packet != NULL) {
            delete[] raw_packet;
            raw_packet = NULL;
        }
        size = -1;
        throw CException(E_ERROR::E_ERR_FAIL_ENCODING_CMD);
    }

    return NULL;
}

// setter
bool CVctrlCMD::set_when(void) {    // set current-time to command.
    struct timespec t_now;
    try {
        // get current-time by REAL-TIME-CLOCK.
        assert( clock_gettime(CLOCK_REALTIME, &t_now) != -1 );

        /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */
        localtime_r((time_t *)&t_now.tv_sec, &when_tm); 
        assert( (when_double = extract_utc2double(t_now)) > 0.0 );
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("set_when0 is failed." );
    }

    return false;
}

bool CVctrlCMD::set_when(E_WEEK next) {  // set next week-day base on cmd-time. (hour/minute/sec is zero-set.)
    struct timespec temp;
    int diff = ((next + 7) - get_week(when_tm)) % 7;

    try{
        if (diff == 0) {
            diff = 7;
        }

        if (diff > 0 ) {
            when_double += (double)(diff * 24 * 3600);
            assert( apply_double2utc(when_double, temp) == true );
            /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */
            localtime_r((time_t *)&temp.tv_sec, &when_tm); 
        }
        assert(next == get_week(when_tm));
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("set_when1 is failed." );
    }

    return false;
}

bool CVctrlCMD::set_when(int hour, int minute, int seconds) {   // set time base on cmd-time.
    assert( hour >= 0 );
    assert( minute >= 0 );
    assert( seconds >= 0 );

    try {
        when_tm.tm_hour = hour;
        when_tm.tm_min = minute;
        when_tm.tm_sec = seconds;
        when_double = (double)(mktime(&when_tm)) + (when_double - (double)((long)when_double));
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("set_when2 is failed." );
    }

    return false;
}

bool CVctrlCMD::set_when(int year, int month, int day, int hour, int minute, int seconds) {   // set next time base on current-time.
    assert( year >= 0 );
    assert( month >= 0 );
    assert( day >= 0 );
    assert( hour >= 0 );
    assert( minute >= 0 );
    assert( seconds >= 0 );

    try {
        when_tm.tm_year = year;
        when_tm.tm_mon = month;
        when_tm.tm_mday = day;
        when_tm.tm_hour = hour;
        when_tm.tm_min = minute;
        when_tm.tm_sec = seconds;
        when_double = (double)(mktime(&when_tm)) + (when_double - (double)((long)when_double));
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("set_when3 is failed." );
    }

    return false;
}

bool CVctrlCMD::append_when(int hour, int minute, int seconds) {
    struct timespec temp;
    assert( hour >= 0 );
    assert( minute >= 0 );
    assert( seconds >= 0 );

    try {
        // sum as seconds.
        seconds = seconds + (minute * 60) + (hour * 3600);
        
        // update double seconds
        when_double += (double)seconds;
        assert( apply_double2utc(when_double, temp) == true );
        /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */
        localtime_r((time_t *)&temp.tv_sec, &when_tm); 
        return true;
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}

bool CVctrlCMD::set_what(E_TARGET valve) {
    assert( E_TARGET::E_VALVE_NULL < valve && valve < E_TARGET::E_VALVE_CNT );
    what = valve;
    return true;
}

bool CVctrlCMD::set_how(const std::string &method) {
    if ( method == OPEN || method == CLOSE ) {
        how = method;
        return true;
    }

    LOGERR("Not supported Value(%s) as method of \"how\".", method.c_str());
    throw CException(E_ERROR::E_ERR_INVALID_VALUE);    
    return false;
}

bool CVctrlCMD::set_why(const std::string &desp) {
    why = desp;
    return true;
}

void CVctrlCMD::set_flag(E_FLAG pos, FlagType value) {
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

void CVctrlCMD::set_state(uint16_t value) {
    _state_ = value;
}

std::string CVctrlCMD::get_sof(const void *payload, ssize_t payload_size) {
    char sof[5] = {0,};
    if(payload == NULL || payload_size <= 4)
        return std::string();

    memcpy(sof, payload, 4);
    return sof;
}

CVctrlCMD::FlagType CVctrlCMD::get_flag(FlagType pos) {
    assert( pos != (FlagType)E_FLAG::E_FLAG_NONE);
    // assert( parsing_complet() == true );
    return _flag_ & pos;
}

uint16_t CVctrlCMD::get_state(void) {
    return _state_;
}

template <>
double CVctrlCMD::get_when<double>(void) { 
    return when_double; 
}

template <>
struct timespec CVctrlCMD::get_when<struct timespec>(void) { 
    struct timespec res;
    assert( apply_double2utc(when_double, res)==true );
    return res; 
}

double CVctrlCMD::get_cur_time(void) {
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

// printer
std::string CVctrlCMD::print_when(void) {  // print when-data for human-readable.
    assert( parsing_complet() == true );
    char when_string[100];

    try {
        strftime(when_string, 100, "CMD-When is [%B %d, %Y] time is [%T]", &when_tm);
        LOGD( "%s", when_string );
        return when_string;
    }
    catch( const std::exception &e ) {
        LOGERR("print_when is failed." );
    }

    return std::string();
}

std::string CVctrlCMD::print_cur_time(void) {  // print current-time for human-readable.
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
E_CMPTIME CVctrlCMD::check_with_curtime(double duty) {   // check whether current-time is over/under/equal with cmd-time.
    assert( parsing_complet() == true );
    struct timespec t_now;
    double d_now = 0.0;

    // get current-time by REAL-TIME-CLOCK.
    assert( clock_gettime(CLOCK_REALTIME, &t_now) != -1 );  
    assert( (d_now = extract_utc2double(t_now)) > 0.0 );

    if ( when_double < (d_now - duty) ) {
        return E_CMPTIME::E_CMPTIME_UNDER;
    }
    else if( when_double > (d_now + duty) ) {
        return E_CMPTIME::E_CMPTIME_OVER;
    }
    else {
        return E_CMPTIME::E_CMPTIME_EQUAL;
    }

    return E_CMPTIME::E_CMPTIME_UNKNOWN;
}

E_CMPTIME CVctrlCMD::check_with_another(CVctrlCMD *cmd, double duty) {
    assert( cmd != NULL );
    assert( parsing_complet() == true );
    double d_another = cmd->get_when();

    if ( when_double < (d_another - duty) ) {
        return E_CMPTIME::E_CMPTIME_UNDER;
    }
    else if( when_double > (d_another + duty) ) {
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
E_WEEK CVctrlCMD::get_week(struct tm &time) {
    return (E_WEEK)time.tm_wday;
}

double CVctrlCMD::extract_utc2double(struct timespec &time) {
    double res = 0.0;

    res += (double)time.tv_sec;
    res += (double)time.tv_nsec / 1000000000.0;

    LOGD( "extracted UTC time = %f", res );
    return res;
}

bool CVctrlCMD::apply_double2utc(double &time, struct timespec &target) {
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

CVctrlCMD::E_TARGET CVctrlCMD::extract_what(Json_DataType &json) {
    std::string type;
    int seq_num = -1;
    E_TARGET which_valve = E_TARGET::E_VALVE_NULL;
    assert(json.get() != NULL);

    // check validation.
    auto objects = json->get_member<json_mng::CMjson>(BODY_KEY_WHAT);
    assert( objects.get() != NULL );
    type = objects->get_member(BODY_KEY_WHAT_TYPE)->c_str();
    assert( type == BODY_VAL_WHAT_TYPE );

    // get valve value.
    seq_num = *(objects->get_member<int>(BODY_KEY_WHAT_SEQ).get());
    switch(seq_num) {
    case 0:
        which_valve = E_TARGET::E_VALVE_LEFT_01;
        break;
    case 1:
        which_valve = E_TARGET::E_VALVE_LEFT_02;
        break;
    case 2:
        which_valve = E_TARGET::E_VALVE_LEFT_03;
        break;
    case 3:
        which_valve = E_TARGET::E_VALVE_LEFT_04;
        break;
    default:
        LOGERR("Invalid BODY_KEY_WHAT_SEQ(%d) data.", seq_num );
        throw CException(E_ERROR::E_ERR_INVALID_VALUE);
        break;
    }
    return which_valve;
}

bool CVctrlCMD::apply_what(Json_DataType &json, E_TARGET valve_seq) {
    Json_DataType json_sub;

    try {
        json_sub = json->set_member(BODY_KEY_WHAT);
        assert( json_sub->set_member(BODY_KEY_WHAT_TYPE, BODY_VAL_WHAT_TYPE) == true );
        assert( json_sub->set_member(BODY_KEY_WHAT_SEQ, (int)valve_seq) == true );
        json->set_member(BODY_KEY_WHAT, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::string CVctrlCMD::extract_how(Json_DataType &json) {
    assert(json.get() != NULL);

    auto objects = json->get_member<json_mng::CMjson>(BODY_KEY_HOW);
    assert( objects.get() != NULL );
    return objects->get_member(BODY_KEY_HOW_METHOD)->c_str();
}

bool CVctrlCMD::apply_how(Json_DataType &json, std::string method) {
    Json_DataType json_sub;

    try {
        json_sub = json->set_member(BODY_KEY_HOW);
        assert( json_sub->set_member(BODY_KEY_HOW_METHOD, method) == true );
        json->set_member(BODY_KEY_HOW, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

std::string CVctrlCMD::extract_why(Json_DataType &json) {
    assert(json.get() != NULL);

    auto objects = json->get_member<json_mng::CMjson>(BODY_KEY_WHY);
    assert( objects.get() != NULL );
    return objects->get_member(BODY_KEY_WHY_DESP)->c_str();
}

bool CVctrlCMD::apply_why(Json_DataType &json, std::string desp) {
    Json_DataType json_sub;

    try {
        json_sub = json->set_member(BODY_KEY_WHY);
        assert( json_sub->set_member(BODY_KEY_WHY_DESP, desp) == true );
        json->set_member(BODY_KEY_WHY, json_sub.get());
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return false;
}

uint32_t CVctrlCMD::gen_random_msg_id(void) {
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
