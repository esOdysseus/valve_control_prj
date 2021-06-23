#include <cassert>
#include <random>
#include <memory>
#include <iostream>
#include <string>

#include <logger.h>
#include <ICommand.h>
#include <Common.h>
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
ICommand::ICommand(std::string my_app_path, std::string my_pvd_id)
: _myself_from_( my_app_path, my_pvd_id ) {
    clear();
    assert(_myself_from_.empty() == false);
}

ICommand::ICommand(const alias::CAlias& myself)
: _myself_from_( myself ) {
    clear();
    assert(_myself_from_.empty() == false);
}

ICommand::~ICommand(void) {
    clear();
}

void ICommand::set_when( std::string type, double start_time, 
                                           Twhen::TEweek week, 
                                           uint32_t period, 
                                           double latency ) {
    _when_.reset();
    _when_ = std::make_shared<Twhen>(type, start_time, week, period, latency);
}

void ICommand::set_how( std::string method, std::string post_method, double costtime ) {
    _how_.reset();
    _how_ = std::make_shared<Thow>(method, post_method, costtime);
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
        throw std::out_of_range("Twho Parsing is not complete.");
    }
    assert( _who_.get() != NULL );
    return *_who_; 
}

ICommand::Twhen& ICommand::when(void) { 
    if( is_parsed() == false ) {
        throw std::out_of_range("Twhen Parsing is not complete.");
    }
    assert( _when_.get() != NULL );
    return *_when_; 
}

ICommand::Twhere& ICommand::where(void) { 
    if( is_parsed() == false ) {
        throw std::out_of_range("Twhere Parsing is not complete.");
    }
    assert( _where_.get() != NULL );
    return *_where_; 
}

ICommand::Twhat& ICommand::what(void) { 
    if( is_parsed() == false ) {
        throw std::out_of_range("Twhat Parsing is not complete.");
    }
    assert( _what_.get() != NULL );
    return *_what_; 
}

ICommand::Thow& ICommand::how(void) { 
    if( is_parsed() == false ) {
        throw std::out_of_range("Thow Parsing is not complete.");
    }
    assert( _how_.get() != NULL );
    return *_how_; 
}

ICommand::Twhy& ICommand::why(void) { 
    if( is_parsed() == false ) {
        throw std::out_of_range("Twhy Parsing is not complete.");
    }
    assert( _why_.get() != NULL );
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
    return json->set_member(JKEY_VERSION, version());
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
    double longitude = Twhere::GPS_NULL;
    double latitude = Twhere::GPS_NULL;
    std::shared_ptr<Twhere> result;
    assert(json.get() != NULL);

    try {
        // check validation.
        auto objects = json->get_member<Json_DataType>(JKEY_WHERE);
        assert( objects.get() != NULL );
        type = objects->get_member(JKEY_WHERE_TYPE);

        // get value.
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

bool ICommand::apply_where(Json_DataType &json, std::shared_ptr<Twhere>& value) {
    Json_DataType json_sub;
    assert( value.get() != NULL );

    try {
        json_sub = json->set_member(JKEY_WHERE);
        assert( json_sub->set_member(JKEY_WHERE_TYPE, value->get_type()) == true );

        if( value->get_gps_long() != Twhere::GPS_NULL && value->get_gps_lat() != Twhere::GPS_NULL ) {
            Json_DataType json_sub2 = json_sub->set_member(JKEY_WHERE_GPS);
            assert( json_sub2->set_member(JKEY_WHERE_GPS_LONG, value->get_gps_long()) == true );
            assert( json_sub2->set_member(JKEY_WHERE_GPS_LAT, value->get_gps_lat()) == true );
            json_sub->set_member(JKEY_WHERE_GPS, json_sub2.get());
        }
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
    int seq_num = -1;
    assert(json.get() != NULL);

    // check validation.
    auto objects = json->get_member<Json_DataType>(JKEY_WHAT);
    assert( objects.get() != NULL );
    type = objects->get_member(JKEY_WHAT_TYPE);

    // get value.
    seq_num = objects->get_member<int>(JKEY_WHAT_SEQ);
    return std::make_shared<Twhat>(type, seq_num);
}

bool ICommand::apply_what(Json_DataType &json, std::shared_ptr<Twhat>& value) {
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

std::shared_ptr<ICommand::Thow> ICommand::extract_how(Json_DataType &json) {
    assert(json.get() != NULL);

    // get method
    auto objects = json->get_member<Json_DataType>(JKEY_HOW);
    assert( objects.get() != NULL );
    auto method = objects->get_member(JKEY_HOW_METHOD);
    auto method_post = objects->get_member(JKEY_HOW_METHOD_POST);
    
    // get costtime
    auto cost_time = objects->get_member<double>(JKEY_HOW_COSTTIME);
    return std::make_shared<Thow>(method, method_post, cost_time);
}

bool ICommand::apply_how(Json_DataType &json, std::shared_ptr<Thow>& value) {
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

    _who_.reset();
    _when_.reset();
    _where_.reset();
    _what_.reset();
    _how_.reset();
    _why_.reset();
}


} // namespace cmd
