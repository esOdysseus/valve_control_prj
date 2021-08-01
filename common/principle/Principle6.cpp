
#include <Principle6.h>
#include <time_kes.h>

#include <logger.h>

namespace principle {

constexpr const char*  CWhen::TYPE_ONECE;
constexpr const char*  CWhen::TYPE_ROUTINE_WEEK;
constexpr const char*  CWhen::TYPE_ROUTINE_DAY;
constexpr const char*  CWhen::TYPE_SPECIAL_TIME;

constexpr const double   CWhen::LATENCY_NULL;
constexpr const char*    CWhen::WEEK_NULL_STR;
constexpr const uint32_t CWhen::PERIOD_NULL;
constexpr const char*    CWhen::DATE_NULL_STR;
constexpr const char*    CWhen::TIME_NULL_STR;
constexpr const double   CWhen::START_TIME_NULL;

const std::map<std::string, CWhen::TEweek> CWhen::MAP_WEEK = {
    { "mon"     , CWhen::TEweek::E_WEEK_MONDAT },
    { "tues"    , CWhen::TEweek::E_WEEK_TUESDAY },
    { "wednes"  , CWhen::TEweek::E_WEEK_WEDNESDAY },
    { "thurs"   , CWhen::TEweek::E_WEEK_THURSDAY },
    { "fri"     , CWhen::TEweek::E_WEEK_FRIDAY },
    { "satur"   , CWhen::TEweek::E_WEEK_SATURDAY },
    { "sun"     , CWhen::TEweek::E_WEEK_SUNDAY },
    { CWhen::WEEK_NULL_STR , CWhen::TEweek::E_WEEK_NONE }
};


/**************************************
 * Public Function definition.
 */
// for decoding
CWhen::CWhen( std::string type, std::string start_date, 
                                std::string run_time,
                                std::string week, 
                                uint32_t period, 
                                double latency,
                                double def_time) {
    try {
        double start_time = def_time;
        TEweek week_e = TEweek::E_WEEK_NONE;

        clear();
        regist_lamda_funcs();

        auto itr = MAP_WEEK.find(week);
        if( itr != MAP_WEEK.end() ) {
            week_e = itr->second;
        }

        if( start_date != DATE_NULL_STR && run_time != TIME_NULL_STR ) {
            start_time = CTime::convert<double>( run_time, start_date.data(), 
                                                 CTime::DEF_TIME_FORMAT, CTime::DEF_DATE_FORMAT );
        }

        apply( type, start_time, week_e, period, latency );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

// for encoding
CWhen::CWhen( std::string type, double start_time, 
                                TEweek week, 
                                uint32_t period, 
                                double latency) {
    try {
        clear();
        regist_lamda_funcs();
        apply( type, start_time, week, period, latency );
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

CWhen::~CWhen( void ) {
    clear();
}

std::string CWhen::get_type(void) {
    return _type_;
}

// decoded usage.
double CWhen::get_start_time(void) {
    return _mm_func_[_type_]();
}

// encoded usage.
double CWhen::get_latency(void) {
    if( _type_ == TYPE_ONECE ) {
        return get_start_time() - CTime::get<double>();
    }
    return LATENCY_NULL;
}

std::string CWhen::get_week(void) {
    if( _week_ == TEweek::E_WEEK_NONE ) {
        return WEEK_NULL_STR;
    }

    for( auto itr=MAP_WEEK.begin(); itr != MAP_WEEK.end(); itr++ ) {
        if( itr->second == _week_ ) {
            LOGI("Found wanted week(%u), It's %s.", _week_, itr->first.data());
            return itr->first;
        }
    }
    return WEEK_NULL_STR;
}

uint32_t CWhen::get_period(void) {
    return _period_;
}

std::string CWhen::get_date(void) {
    if ( _start_time_ != START_TIME_NULL ) {
        return CTime::print(_start_time_, CTime::DEF_DATE_FORMAT);
    }
    return DATE_NULL_STR;
}

std::string CWhen::get_time(void) {
    if ( _start_time_ != START_TIME_NULL ) {
        return CTime::print(_start_time_, CTime::DEF_TIME_FORMAT);
    }
    return DATE_NULL_STR;
}

double CWhen::get_next_period( double base_time ) {
    double next_time = START_TIME_NULL;
    try {
        if( _type_ == TYPE_ROUTINE_WEEK ) {
            next_time = get_next_week( base_time, _week_, _period_);
        }
        else if( _type_ == TYPE_ROUTINE_DAY ) {
            next_time = get_next_day( base_time, _period_ );
        }
        else {
            std::string err = "Not Supported when-type (" + _type_ + ")";
            throw std::out_of_range(err);
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return next_time;
}


/**************************************
 * Private Function definition.
 */
void CWhen::clear(void) {
    _flag_apply_done_ = false;

    _type_.clear();
    _start_time_ = START_TIME_NULL;
    _week_ = TEweek::E_WEEK_NONE;
    _period_ = PERIOD_NULL;
}

void CWhen::apply( std::string type, double start_time,
                                     TEweek week, 
                                     uint32_t period,
                                     double latency) {
    LOGD("Enter");

    try {
        if( _flag_apply_done_ == true ) {
            LOGI("Already appling is done.");
            return;
        }
        check_validation(type, start_time, week, period, latency);

        _type_ = type;
        _week_ = week;
        _period_ = period;
        _start_time_ = start_time;

        if( _start_time_ == START_TIME_NULL ) {
            _start_time_ = CTime::get<double>();
        }

        if( latency != LATENCY_NULL && latency > 0.0 ) {
            _start_time_ += latency;
        }

        _flag_apply_done_ = true;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CWhen::regist_lamda_funcs(void) {
    LOGD("Enter");

    _mm_func_[TYPE_ONECE] = [this](void) -> double {
        if( _flag_apply_done_ == false ) {
            return START_TIME_NULL;
        }
        return this->_start_time_;
    };

    _mm_func_[TYPE_ROUTINE_WEEK] = [this](void) -> double {
        if( _flag_apply_done_ == false ) {
            return START_TIME_NULL;
        }

        try {
            double now = CTime::get<double>();

            while( now > this->_start_time_ ) {
                this->_start_time_ = CWhen::get_next_week( this->_start_time_, 
                                                           this->_week_, 
                                                           this->_period_ );
            }
        }
        catch( const std::exception& e ) {
            LOGERR("%s", e.what());
            throw e;
        }
        return this->_start_time_;
    };

    _mm_func_[TYPE_ROUTINE_DAY] = [this](void) -> double {
        if( _flag_apply_done_ == false ) {
            return START_TIME_NULL;
        }

        try {
            double now = CTime::get<double>();

            while( now > this->_start_time_ ) {
                this->_start_time_ = CWhen::get_next_day( this->_start_time_, 
                                                          this->_period_ );
            }
        }
        catch( const std::exception& e ) {
            LOGERR("%s", e.what());
            throw e;
        }
        return this->_start_time_;
    };

    _mm_func_[TYPE_SPECIAL_TIME] = _mm_func_[TYPE_ONECE];

    LOGD("Exit");
}

void CWhen::check_validation( std::string type, double start_time,
                                                TEweek week, 
                                                uint32_t period,
                                                double latency) {
    LOGD("Enter");

    try {
        double now = CTime::get<double>();

        if ( type != TYPE_ONECE && 
             type != TYPE_ROUTINE_WEEK && 
             type != TYPE_ROUTINE_DAY && 
             type != TYPE_SPECIAL_TIME ) {
            std::string err = "Not Supported type(" + type + ") is inserted.";
            throw std::out_of_range(err);
        }

        if( type != TYPE_ONECE && start_time == START_TIME_NULL ) {
            std::string err = "start_time is needed in \"" + type + "\" type.";
            throw std::invalid_argument(err);
        }

        if( type == TYPE_ROUTINE_WEEK ) {
            if( TEweek::E_WEEK_NONE >= week || week >= TEweek::E_WEEK_CNT ) {
                std::string err = "week support in \"mon, tues, wednes, thurs, fri, satur, sun\".";
                throw std::out_of_range(err);
            }
        }

        if( (type == TYPE_ROUTINE_WEEK || type == TYPE_ROUTINE_DAY) && period == PERIOD_NULL ) {
            std::string err = "period is needed in \"" + type + "\" type.";
            throw std::invalid_argument(err);
        }

        if( type == TYPE_ONECE ) {
            if( start_time == START_TIME_NULL && latency == LATENCY_NULL ) {
                std::string err = "start-time xor latency is needed in \"" + type + "\" type.";
                throw std::invalid_argument(err);
            }
            else if( start_time != START_TIME_NULL && latency != LATENCY_NULL ) {
                std::string err = "start-time xor latency is needed in \"" + type + "\" type.";
                throw std::invalid_argument(err);
            }
            else if( start_time != START_TIME_NULL && start_time <= now ) {
                std::string err = "Future start-time is needed in \"" + type + "\" type.";
                throw std::invalid_argument(err);
            }
            else if( latency != LATENCY_NULL && latency <= 0.0) {
                std::string err = "Positive latency is needed in \"" + type + "\" type.";
                throw std::invalid_argument(err);
            }
        }

        if( type == TYPE_SPECIAL_TIME && start_time <= now ) {
            std::string err = "Future start-time is needed in \"" + type + "\" type.";
            throw std::invalid_argument(err);
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

// reference function.
double CWhen::get_next_week( double base_time, TEweek week, uint32_t period ) {
    try {
        uint32_t need_days = 0;
        TEweek base_week = TEweek::E_WEEK_NONE;

        if( base_time == START_TIME_NULL ) {
            throw std::invalid_argument("base_time is invalid value(NULL).");
        }

        if( TEweek::E_WEEK_NONE >= week || week >= TEweek::E_WEEK_CNT ) {
            std::string err = "week is invalid value(" + std::to_string(week) + "). it support in \"mon, tues, wednes, thurs, fri, satur, sun\".";
            throw std::out_of_range(err);
        }

        if( period == PERIOD_NULL ) {
            throw std::invalid_argument("period is invalid value(NULL).");
        }

        // calculate need-days for next-week.
        base_week = CTime::get_week( base_time );
        need_days = ((week + 7) - base_week) % 7;
        if (need_days == 0) {
            need_days = 7;
        }
        // calculate need-days for target-week.
        need_days += 7*(period-1);

        return base_week + static_cast<double>( 24 * 3600 * need_days );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return START_TIME_NULL;
}

double CWhen::get_next_day( double base_time, uint32_t period ) {
    try {
        if( base_time == START_TIME_NULL ) {
            throw std::invalid_argument("base_time is invalid value(NULL).");
        }

        if( period == PERIOD_NULL ) {
            throw std::invalid_argument("period is invalid value(NULL).");
        }

        return base_time + static_cast<double>( 24 * 3600 * period );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return START_TIME_NULL;
}


}   // principle