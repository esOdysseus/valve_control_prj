#include <cassert>
#include <string.h>
#include <type_traits>

#include <logger.h>
#include <time_kes.h>
#include <CException.h>


namespace time_pkg {

/** GET functions */
template <>
struct timespec CTime::get(void) {
    struct timespec t_now;
    try {
        if( clock_gettime(CLOCK_REALTIME, &t_now) == -1 ) {
            LOGERR("clock_gettime is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_GET_TIME);
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return t_now;
}

template <>
tm CTime::get(void) {
    tm res = {0,};
    try {
        struct timespec t_now = CTime::get<struct timespec>();
        localtime_r((time_t *)&t_now.tv_sec, &res); 
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return res;
}

template <>
time_t CTime::get(void) {
    try {
        struct timespec t_now = CTime::get<struct timespec>();
        return t_now.tv_sec;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return 0;
}

template <>
double CTime::get(void) {
    try {
        struct timespec t_now = CTime::get<struct timespec>();
        return CTime::convert<double>(t_now);
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return 0.0;
}


/** GET_NEXT functions */
template struct timespec CTime::get_next<struct timespec, struct timespec>(struct timespec &time);
template double CTime::get_next<struct timespec, double>(struct timespec &time);
template time_t CTime::get_next<struct timespec, time_t>(struct timespec &time);
template tm CTime::get_next<struct timespec, tm>(struct timespec &time);
template struct timespec CTime::get_next<double, struct timespec>(double &time);
template double CTime::get_next<double, double>(double &time);
template time_t CTime::get_next<double, time_t>(double &time);
template tm CTime::get_next<double, tm>(double &time);
template struct timespec CTime::get_next<time_t, struct timespec>(time_t &time);
template double CTime::get_next<time_t, double>(time_t &time);
template time_t CTime::get_next<time_t, time_t>(time_t &time);
template tm CTime::get_next<time_t, tm>(time_t &time);
template struct timespec CTime::get_next<tm, struct timespec>(tm &time);
template double CTime::get_next<tm, double>(tm &time);
template time_t CTime::get_next<tm, time_t>(tm &time);
template tm CTime::get_next<tm, tm>(tm &time);

template <typename _IN_TYPE_, typename T>
T CTime::get_next(_IN_TYPE_ &time) {
    double latency = CTime::convert<double>(time);
    return CTime::convert<T>( CTime::get<double>()+latency );
}

template struct timespec CTime::get_next<struct timespec>(unsigned int index, E_WEEK_TIME_VALVE week, const char* cost_time, time_t * base_time, const char* format, bool include_today);
template double CTime::get_next<double>(unsigned int index, E_WEEK_TIME_VALVE week, const char* cost_time, time_t * base_time, const char* format, bool include_today);
template time_t CTime::get_next<time_t>(unsigned int index, E_WEEK_TIME_VALVE week, const char* cost_time, time_t * base_time, const char* format, bool include_today);
template tm CTime::get_next<tm>(unsigned int index, E_WEEK_TIME_VALVE week, const char* cost_time, time_t * base_time, const char* format, bool include_today);

template <typename T>
T CTime::get_next(unsigned int index,      E_WEEK_TIME_VALVE week, 
                  const char* cost_time,   time_t * base_time, 
                  const char* format,      bool include_today) {
    time_t _base_time_ = 0;
    int diff_day = 0;
    assert(format != NULL);
    assert(index < CTime::MAX_WEEK_CNT);

    try {
        // create base time.
        if( base_time != NULL )
            _base_time_ = *base_time;
        else
            _base_time_ = get<time_t>();
        
        // calculate duration-day from base-time.
        diff_day = calc_duration_day(index, week, _base_time_, include_today);
    
        if (diff_day > -1) { 
            _base_time_ += (time_t)(diff_day * 24 * 3600);
            if( cost_time ) {
                // set absolute-time without date-data.
                struct tm cost_tm = CTime::convert<tm>(cost_time, NULL, format);
                struct tm base_tm = CTime::convert<tm>(_base_time_);
                base_tm.tm_hour = cost_tm.tm_hour;
                base_tm.tm_min = cost_tm.tm_min;
                base_tm.tm_sec = cost_tm.tm_sec;
                _base_time_ = CTime::convert<time_t>(base_tm);
            }

            if ( CTime::elapsed_time(_base_time_, get<time_t>()) < 0.0 ) {
                _base_time_ += (time_t)(7 * 24 * 3600);
            }
        }
        else {
            LOGERR("calc_duration_day is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_CALCULATE_TIME);
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return CTime::convert<T>(_base_time_);
}

/** GET-WEEK functions */
template <>
inline E_WEEK_TIME_VALVE CTime::get_week(struct timespec &time) {
    tm temp = convert<struct tm>(time);
    return (E_WEEK_TIME_VALVE)temp.tm_wday;
}

template <>
inline E_WEEK_TIME_VALVE CTime::get_week(double &time) {
    tm temp = convert<struct tm>(time);
    return (E_WEEK_TIME_VALVE)temp.tm_wday;
}

template <>
inline E_WEEK_TIME_VALVE CTime::get_week(time_t &time) {
    tm temp = convert<struct tm>(time);
    return (E_WEEK_TIME_VALVE)temp.tm_wday;
}

template <>
inline E_WEEK_TIME_VALVE CTime::get_week(tm &time) {
    return (E_WEEK_TIME_VALVE)time.tm_wday;
}

inline E_WEEK_TIME_VALVE CTime::get_week(void) {
    tm temp = CTime::get<tm>();
    return (E_WEEK_TIME_VALVE)temp.tm_wday;
}


/** SET functions */
template <>
bool CTime::set(time_t time) {
    try {
        // set local system-time. (unit seconds)
        if ( stime(&time) == -1 ) {
            LOGERR("Setting of 'local Sys-Time' is failed.");
            LOGERR("%d: %s", errno, strerror(errno));
            if( errno == 1 ) {
                LOGW("You have to run cmd_scheduler with 'root' permission.");
            }
            throw CException(E_ERROR::E_ERR_FAIL_TIME_UPDATE);
        }
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException &>(e);
    }
    return false;
}

template <>
bool CTime::set(struct timespec &time) {
    return set<time_t>(time.tv_sec);
}

template <>
bool CTime::set(double time) {
    return set<time_t>( (long)time );
}

template <>
bool CTime::set(tm &time) {
    return set<time_t>(mktime(&time));
}


/** CONVERT functions */
// -------------------- IN: struct timespec ------------------------
template <typename  T>
inline CTime::TTYPE_timespec<T> CTime::convert(struct timespec &time) {
    return time;
}

template <typename  T>
CTime::TTYPE_double<T> CTime::convert(struct timespec &time) {
    double res = 0.0;
    try {
        res += (double)time.tv_sec;
        res += (double)time.tv_nsec / 1000000000.0;
        LOGD( "extracted UTC time = %f", res );
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw CException(E_ERROR::E_ERR_FAIL_CONVERTING_TIME);
    }
    return res;
}

template <typename  T>
inline CTime::TTYPE_time_t<T> CTime::convert(struct timespec &time) {
    return (time_t)(time.tv_nsec);
}

template <typename  T>
CTime::TTYPE_tm<T> CTime::convert(struct timespec &time) {
    time_t temp = (time_t)(time.tv_nsec);
    return convert<struct tm>(temp);
}

// -------------------- IN: double ------------------------
template <typename  T>
CTime::TTYPE_timespec<T> CTime::convert(double time) {
    struct timespec target = {0, 0};
    try {
        target.tv_sec = (long)time;
        target.tv_nsec = (time - (long)time) * 1000000000L;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw CException(E_ERROR::E_ERR_FAIL_CONVERTING_TIME);
    }
    return target;
}

template <typename  T>
inline CTime::TTYPE_double<T> CTime::convert(double time) {
    return time;
}

template <typename  T>
inline CTime::TTYPE_time_t<T> CTime::convert(double time) {
    return (time_t)time;
}

template <typename  T>
CTime::TTYPE_tm<T> CTime::convert(double time) {
    tm res = {0,};
    try{
        time_t temp = (time_t)time;
        localtime_r(&temp, &res); 
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return res;
}

// -------------------- IN: time_t ------------------------
template <typename  T>
CTime::TTYPE_timespec<T> CTime::convert(time_t &time) {
    struct timespec temp = {0,};

    temp.tv_sec = time;
    temp.tv_nsec = 0;
    return temp;
}

template <typename  T>
inline CTime::TTYPE_double<T> CTime::convert(time_t &time) {
    return (double)time;
}

template <typename  T>
inline CTime::TTYPE_time_t<T> CTime::convert(time_t &time) {
    return time;
}

template <typename  T>
CTime::TTYPE_tm<T> CTime::convert(time_t &time) {
    struct tm temp = {0,};
    try{
        localtime_r(&time, &temp); 
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return temp;
}

// -------------------- IN: struct tm ------------------------
template <typename  T>
CTime::TTYPE_timespec<T> CTime::convert(tm &time) {
    struct timespec temp = {0,};
    try {
        temp.tv_sec = mktime(&time);
        temp.tv_nsec = 0;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return temp;
}

template <typename  T>
inline CTime::TTYPE_double<T> CTime::convert(tm &time) {
    return (double)mktime(&time);
}

template <typename  T>
inline CTime::TTYPE_time_t<T> CTime::convert(tm &time) {
    return mktime(&time);
}

template <typename  T>
inline CTime::TTYPE_tm<T> CTime::convert(tm &time) {
    return time;
}

// -------------------- IN: string ------------------------
template struct timespec CTime::convert<struct timespec>(std::string time, const char* date, std::string time_format, std::string date_format);
template double CTime::convert<double>(std::string time, const char* date, std::string time_format, std::string date_format);
template time_t CTime::convert<time_t>(std::string time, const char* date, std::string time_format, std::string date_format);
template tm CTime::convert<tm>(std::string time, const char* date, std::string time_format, std::string date_format);

template <typename T>
T CTime::convert(std::string time, const char* date, 
                 std::string time_format, std::string date_format) {
    tm time_tm = {0,};
    assert(time.empty() == false && time_format.empty() == false);

    try {
        if( date != NULL ) {
            time += date;
            time_format += date_format;
        }

        if( strptime(time.c_str(), time_format.c_str(), &time_tm) == NULL ) {
            LOGERR("strptime is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_CONVERTING_TIME);
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }

    return CTime::convert<T>(time_tm);
}
                            

/** ELAPSED_TIME functions */
template double CTime::elapsed_time(struct timespec, struct timespec);
template double CTime::elapsed_time(struct timespec, double);
template double CTime::elapsed_time(struct timespec, time_t);
template double CTime::elapsed_time(struct timespec, tm);
template double CTime::elapsed_time(time_t, struct timespec);
template double CTime::elapsed_time(time_t, double);
template double CTime::elapsed_time(time_t, time_t);
template double CTime::elapsed_time(time_t, tm);
template double CTime::elapsed_time(tm, struct timespec);
template double CTime::elapsed_time(tm, double);
template double CTime::elapsed_time(tm, time_t);
template double CTime::elapsed_time(tm, tm);
template double CTime::elapsed_time(double, struct timespec);
template double CTime::elapsed_time(double, time_t);
template double CTime::elapsed_time(double, tm);

template <>
inline double CTime::elapsed_time(double time_first, double time_second) {
    return time_first - time_second;
}

template <typename _IN_TYPE_01_, typename _IN_TYPE_02_>
double CTime::elapsed_time(_IN_TYPE_01_ time_first, _IN_TYPE_02_ time_second) {
    double time_01 = CTime::convert<double>(time_first);
    double time_02 = CTime::convert<double>(time_second);
    return time_01 - time_02;
}


/** PRINT functions */
template std::string CTime::print(struct timespec &time, std::string format);
template std::string CTime::print(struct timespec time, std::string format);
template std::string CTime::print(double time, std::string format);
template std::string CTime::print(time_t time, std::string format);
template std::string CTime::print(struct tm &time, std::string format);
template std::string CTime::print(struct tm time, std::string format);

template <typename _IN_TYPE_>
std::string CTime::print(_IN_TYPE_ time, std::string format) {
    unsigned int length = format.length() + 25;
    char buffer[length] ={0,};
    tm _time_ = {0,};

    try {
        _time_ = CTime::convert<tm>(time);
        if( strftime(buffer, length, format.c_str(), &_time_) <= 0 ) {
            LOGERR("strftime is failed.");
            throw CException(E_ERROR::E_ERR_FAIL_PRINT_TIME);
        }
        return std::string(buffer);
    }
    catch ( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return std::string();
}


/*************************************
 * Definition of Private Function.
 */
int CTime::calc_duration_day(unsigned int index, E_WEEK_TIME_VALVE week, time_t base_time, bool include_today) {
    int diff_day = -1;
    try {
        diff_day = ((week + 7) - CTime::get_week(base_time)) % 7;
        if (include_today == false && diff_day == 0) {
            diff_day = 7;
        }

        diff_day += (index * 7);
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw e;
    }
    return diff_day;
}



}   // namespace time_pkg
