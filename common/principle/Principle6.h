#ifndef _PRINCIPLE_6_CLASSES_H_
#define _PRINCIPLE_6_CLASSES_H_

#include <map>
#include <string>
#include <functional>
#include <time_kes.h>

namespace principle {


/*****************
 * Who Class
 */
class CWho {
public:
    CWho( std::string app_path, std::string pvd_id, std::string func_id="none" ) {
        _app_path_ = app_path;
        _pvd_id_ = pvd_id;
        _func_id_ = func_id;
    }

    ~CWho( void ) {
        _app_path_.clear();
        _pvd_id_.clear();
        _func_id_.clear();
    }

    std::string get_app(void) { return _app_path_;  }

    std::string get_pvd(void) { return _pvd_id_; }

    std::string get_func(void) { return _func_id_; }

private:
    CWho(void) = delete;

private:
    std::string _app_path_;
    std::string _pvd_id_;
    std::string _func_id_;

};

/*****************
 * When Class
 */
class CWhen {
public:
    static constexpr const char*    LATENCY_NULL_STR    = "-1.0";
    static constexpr const double   LATENCY_NULL        = -1.0;
    static constexpr const char*    WEEK_NULL_STR       = "none";
    static constexpr const char*    PERIOD_NULL_STR     = "0";
    static constexpr const uint32_t PERIOD_NULL         = 0;
    static constexpr const char*    DATE_NULL_STR       = "none";
    static constexpr const char*    TIME_NULL_STR       = "none";

    using TEweek = time_pkg::E_WEEK_TIME_VALVE;

private:
    using CTime = time_pkg::CTime;
    using TFcalc = std::function<double(void)>;     /*calculator of start_time*/

public:
    // for decoding
    CWhen( std::string type, std::string start_date = DATE_NULL_STR, 
                             std::string run_time = TIME_NULL_STR,
                             std::string week = WEEK_NULL_STR, 
                             std::string period = PERIOD_NULL_STR, 
                             std::string latency = LATENCY_NULL_STR);

    // for encoding
    CWhen( std::string type, double start_time, 
                             TEweek week = TEweek::E_WEEK_NONE, 
                             uint32_t period = PERIOD_NULL, 
                             double latency = LATENCY_NULL);

    ~CWhen( void );

    std::string get_type(void);

    // decoded usage.
    double get_start_time(void);

    // encoded usage.
    double get_latency(void);

    std::string get_week(void);

    uint32_t get_period(void);

    std::string get_date(void);

    std::string get_time(void);

    // reference function.
    static double get_next_week( double base_time, TEweek week, uint32_t period );

    static double get_next_day( double base_time, uint32_t period );

private:
    CWhen(void) = delete;

    void clear(void);

    void apply( std::string type, double start_time,
                                  TEweek week, 
                                  uint32_t period,
                                  double latency);

    void regist_lamda_funcs(void);

    void check_validation( std::string type, double start_time,
                                             TEweek week, 
                                             uint32_t period,
                                             double latency);

private:
    std::map<std::string/*type*/, TFcalc> _mm_func_;

    bool _flag_apply_done_;

    std::string _type_;
    double _start_time_;

    TEweek _week_;
    uint32_t _period_;

    static const std::map<std::string, TEweek> MAP_WEEK;

};

/*****************
 * Where Class
 */
class CWhere {
public:
    static constexpr const double GPS_NULL = -1.0;

public:
    CWhere( std::string type, std::string gps_long="-1.0", std::string gps_lat="-1.0" ) {
        _type_ = type;
        _gps_long_ = std::stod( gps_long, nullptr );
        _gps_lat_ = std::stod( gps_lat, nullptr );
    }

    ~CWhere( void ) {
        _type_.clear();
        _gps_long_ = GPS_NULL;
        _gps_lat_ = GPS_NULL;
    }

    std::string get_type(void) {    return _type_;  }

    double get_gps_long(void) { return _gps_long_; }

    double get_gps_lat(void) { return _gps_lat_; }

private:
    CWhere(void) = delete;

private:
    std::string _type_;
    double _gps_long_;
    double _gps_lat_;

};

/*****************
 * What Class
 */
class CWhat {
public:
    static constexpr const int WHICH_NULL = -1;

public:
    CWhat( std::string type, int which ) {
        _type_ = type;
        _which_ = which;
    }

    ~CWhat( void ) {
        _type_.clear();
        _which_ = WHICH_NULL;
    }

    std::string get_type(void) {    return _type_;  }

    int get_which(void) { return _which_; }

private:
    CWhat(void) = delete;

private:
    std::string _type_;
    int _which_;

};

/*****************
 * How Class
 */
class CHow {
public:
    static constexpr const double COSTTIME_NULL = -1.0;
    static constexpr const char* METHOD_NULL = "none";

public:
    CHow( std::string method, std::string method_post, std::string cost_time ) {
        _method_ = method;
        _post_method_ = method_post;
        _cost_time_ = std::stod( cost_time, nullptr );
    }

    CHow( std::string method, std::string method_post=METHOD_NULL, double cost_time=COSTTIME_NULL ) {
        _method_ = method;
        _post_method_ = method_post;
        _cost_time_ = cost_time;
    }

    ~CHow( void ) {
        _method_.clear();
        _post_method_.clear();
        _cost_time_ = COSTTIME_NULL;
    }

    std::string get_method(void) {    return _method_;  }

    double get_costtime(void) { return _cost_time_; }

    std::string get_post_method(void) {    return _post_method_;  }

private:
    CHow(void) = delete;

private:
    std::string _method_;
    std::string _post_method_;
    double _cost_time_;         // desp: It's that cost time for operating of method. after that post-method is operated.

};


}   // principle


#endif // _PRINCIPLE_6_CLASSES_H_