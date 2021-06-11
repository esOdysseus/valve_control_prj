#ifndef _TIME_BY_KES_H_
#define _TIME_BY_KES_H_

#include <string>
#include <time.h>

namespace time_pkg {

/** WHEN-week for Valve */
typedef enum E_WEEK_TIME_VALVE {
    E_WEEK_NONE = -1,
    E_WEEK_SUNDAY = 0,
    E_WEEK_MONDAT = 1,
    E_WEEK_TUESDAY = 2,
    E_WEEK_WEDNESDAY = 3,
    E_WEEK_THURSDAY = 4,
    E_WEEK_FRIDAY = 5,
    E_WEEK_SATURDAY = 6,
    E_WEEK_CNT = 7
} E_WEEK_TIME_VALVE;

class CTime {
public:
    template <typename T>
    using TTYPE_timespec = typename std::enable_if< std::is_same< T, struct timespec >::value, T >::type;
    template <typename T>
    using TTYPE_double = typename std::enable_if< std::is_same< T, double >::value, T >::type;
    template <typename T>
    using TTYPE_time_t = typename std::enable_if< std::is_same< T, time_t >::value, T >::type;
    template <typename T>
    using TTYPE_tm = typename std::enable_if< std::is_same< T, struct tm >::value, T >::type;

public:
    static constexpr const char* DEF_TIME_FORMAT = "%T";
    static constexpr const char* DEF_DATE_FORMAT = "%Y-%m-%d";
    static constexpr unsigned int MAX_WEEK_CNT = 10;

public:
    /** GET functions */
    template <typename T>
    static T get(void);

    /** GET-NEXT functions */
    template <typename _IN_TYPE_, typename T>
    static T get_next(_IN_TYPE_ &time);

    // template <typename T>
    // static T get_next(double latency);

    template <typename T>
    static T get_next(unsigned int index, E_WEEK_TIME_VALVE week, 
                      const char* time,   time_t * base_time=NULL, 
                      const char* format=DEF_TIME_FORMAT, bool include_today=true);

    /** GET-WEEK functions */
    template <typename _IN_TYPE_>
    static E_WEEK_TIME_VALVE get_week(_IN_TYPE_ &time);

    static E_WEEK_TIME_VALVE get_week(void);

    /** SET functions */
    template <typename _IN_TYPE_>
    static bool set(_IN_TYPE_ time);

    /** CONVERT functions */
    template <typename T>
    static TTYPE_timespec<T> convert(struct timespec &time);

    template <typename T>
    static TTYPE_double<T> convert(struct timespec &time);

    template <typename T>
    static TTYPE_time_t<T> convert(struct timespec &time);

    template <typename T>
    static TTYPE_tm<T> convert(struct timespec &time);


    template <typename T>
    static TTYPE_timespec<T> convert(double time);

    template <typename T>
    static TTYPE_double<T> convert(double time);

    template <typename T>
    static TTYPE_time_t<T> convert(double time);

    template <typename T>
    static TTYPE_tm<T> convert(double time);


    template <typename T>
    static TTYPE_timespec<T> convert(time_t &time);

    template <typename T>
    static TTYPE_double<T> convert(time_t &time);

    template <typename T>
    static TTYPE_time_t<T> convert(time_t &time);

    template <typename T>
    static TTYPE_tm<T> convert(time_t &time);


    template <typename T>
    static TTYPE_timespec<T> convert(tm &time);

    template <typename T>
    static TTYPE_double<T> convert(tm &time);

    template <typename T>
    static TTYPE_time_t<T> convert(tm &time);

    template <typename T>
    static TTYPE_tm<T> convert(tm &time);


    template <typename T>
    static T convert(std::string time, const char* date=NULL, 
                     std::string time_format=DEF_TIME_FORMAT, 
                     std::string date_format=DEF_DATE_FORMAT);

    /** ELAPSED-TIME functions */
    template <typename _IN_TYPE_01_, typename _IN_TYPE_02_>
    static double elapsed_time(_IN_TYPE_01_ time_first, _IN_TYPE_02_ time_second);

    /** PRINT functions */
    template <typename _IN_TYPE_>
    static std::string print(_IN_TYPE_ time, std::string date_format);

    static std::string print_nanotime(std::string format=DEF_TIME_FORMAT);

private:
    CTime(void) = delete;

    ~CTime(void) = delete;

    static int calc_duration_day(unsigned int index, 
                                 E_WEEK_TIME_VALVE week, 
                                 time_t base_time,
                                 bool include_today=true);

    static constexpr const uint32_t BUFFER_MAX_SIZE = 64;

};


} // namespace time_pkg


#endif // _TIME_BY_KES_H_
