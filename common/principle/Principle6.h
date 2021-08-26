#ifndef _PRINCIPLE_6_CLASSES_H_
#define _PRINCIPLE_6_CLASSES_H_

#include <map>
#include <string>
#include <functional>
#include <time_kes.h>
#include <iostream>

#include <json_manipulator.h>
#include <contents/Contents.h>

namespace principle {


/*****************
 * Who Class
 */
class CWho {
public:
    static constexpr const char* STR_NULL = "none";

public:
    CWho( std::string app_path, std::string pvd_id, std::string func_id=STR_NULL ) {
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
    static constexpr const char*    TYPE_ONECE = "one-time";
    static constexpr const char*    TYPE_ROUTINE_WEEK = "routine.week";
    static constexpr const char*    TYPE_ROUTINE_DAY = "routine.day";
    static constexpr const char*    TYPE_SPECIAL_TIME = "specific";

    static constexpr const double   LATENCY_NULL        = -1.0;
    static constexpr const char*    WEEK_NULL_STR       = "none";
    static constexpr const uint32_t PERIOD_NULL         = 0;
    static constexpr const char*    DATE_NULL_STR       = "none";
    static constexpr const char*    TIME_NULL_STR       = "none";
    static constexpr const double   START_TIME_NULL     = 0.0;

    using TEweek = time_pkg::E_WEEK_TIME_VALVE;

private:
    using CTime = time_pkg::CTime;
    using TFcalc = std::function<double(void)>;     /*calculator of start_time*/

public:
    // for decoding
    CWhen( std::string type, std::string start_date = DATE_NULL_STR, 
                             std::string run_time = TIME_NULL_STR,
                             std::string week = WEEK_NULL_STR, 
                             uint32_t period = PERIOD_NULL, 
                             double latency = LATENCY_NULL,
                             double def_time = START_TIME_NULL);

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

    double get_next_period( double base_time );

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

    // reference function.
    static double get_next_week( double base_time, TEweek week, uint32_t period );

    static double get_next_day( double base_time, uint32_t period );

private:
    std::map<std::string/*type*/, TFcalc> _mm_func_;

    bool _flag_apply_done_;

    std::string _type_;
    double _start_time_;

    TEweek _week_;
    uint32_t _period_;

    static const std::map<std::string, TEweek> MAP_WEEK;

};


class HContentsBase {
public:
    HContentsBase(void) = default;

    virtual ~HContentsBase(void) {
        _type_.clear();
        _m_contents_.reset();
    }

    std::string get_type(void) {
        return _type_;
    }

    Json_DataType encode( void ) {
        Json_DataType result;
        if( _m_contents_.get() != NULL ) {
            result = _m_contents_->encode();
        }
        return result;
    }

protected:
    std::string _type_;
    std::shared_ptr<IBaseContents> _m_contents_;

};

/*****************
 * Where Class
 */
class CWhere: public HContentsBase {
public:
    static constexpr const double   GPS_NULL = -1.0;
    static constexpr const char*    TYPE_GPS = "center.gps";
    static constexpr const char*    TYPE_DB = "db";
    static constexpr const char*    TYPE_UNKNOWN = "unknown";
    static constexpr const char*    TYPE_NOTCARE = "dont.care";

public:
    CWhere( std::string type, Json_DataType &json );

    CWhere( std::string type, double gps_long, double gps_lat );

    CWhere( std::string type, Tdb_type db_type, std::string db_path, std::string db_table );

    CWhere( std::string type );

    ~CWhere( void ) = default;

    /** GPS-API functions. */
    double& gps_longitude(void);

    double& gps_latitude(void);

    /** DB-API functions. */
    Tdb_type& db_type(void);

    std::string& db_path(void);

    std::string& db_table(void);

private:
    CWhere(void) = delete;

    template<cWhereGPS::Tcontents T>
    auto get_ref_gps(void) -> typename std::add_lvalue_reference< decltype(cWhereGPS::Ttype<T>()) >::type {
        if( _type_ != TYPE_GPS ) {
            throw std::out_of_range("Not Supported GPS-API.");
        }

        auto contents = std::dynamic_pointer_cast<cWhereGPS>(_m_contents_);
        return contents->get<T>();
    }

    template<cWhereDB::Tcontents T>
    auto get_ref_db(void) -> typename std::add_lvalue_reference< decltype(cWhereDB::Ttype<T>()) >::type {
        if( _type_ != TYPE_DB ) {
            throw std::out_of_range("Not Supported DB-API.");
        }

        auto contents = std::dynamic_pointer_cast<cWhereDB>(_m_contents_);
        return contents->get<T>();
    }

};

/*****************
 * What Class
 */
class CWhat: public HContentsBase {
public:
    static constexpr const int      WHICH_NULL = -1;
    static constexpr const char*    TYPE_VALVE = "valve.swc";
    static constexpr const char*    TYPE_DB = "db";

    using TDtype = std::map< std::string, std::shared_ptr<std::map<std::string, std::string>> >;

public:
    CWhat( std::string type, Json_DataType &json );

    CWhat( std::string type, int which );

    CWhat( std::string type, Tdb_data data_type, TDtype& data );

    ~CWhat(void) = default;

    /** VALVE-API functions. */
    uint32_t& valve_which(void);

    /** DB-API functions. */
    Tdb_data& db_type(void);

    TDtype& db_target(void);

private:
    CWhat(void) = delete;

    template<cWhatVALVE::Tcontents T>
    auto get_ref_valve(void) -> typename std::add_lvalue_reference< decltype(cWhatVALVE::Ttype<T>()) >::type {
        if( _type_ != TYPE_VALVE ) {
            throw std::out_of_range("Not Supported VALVE-API.");
        }

        auto contents = std::dynamic_pointer_cast<cWhatVALVE>(_m_contents_);
        return contents->get<T>();
    }

    template<cWhatDB::Tcontents T>
    auto get_ref_db(void) -> typename std::add_lvalue_reference< decltype(cWhatDB::Ttype<T>()) >::type {
        if( _type_ != TYPE_DB ) {
            throw std::out_of_range("Not Supported DB-API.");
        }

        auto contents = std::dynamic_pointer_cast<cWhatDB>(_m_contents_);
        return contents->get<T>();
    }

};

/*****************
 * How Class
 */
class CHow: public HContentsBase {
public:
    static constexpr const double   COSTTIME_NULL = -1.0;
    static constexpr const char*    METHOD_NULL = "none";
    static constexpr const char*    TYPE_VALVE = "valve.swc";
    static constexpr const char*    TYPE_DB = "db";

    using TDtype = std::map<std::string, std::string>;

public:
    CHow( std::string type, Json_DataType &json );

    CHow( std::string type, Tvalve_method method_pre, double costtime, Tvalve_method method_post );

    CHow( std::string type, Tdb_method method, TDtype& condition );

    ~CHow(void) = default;

    /** VALVE-API functions. */
    Tvalve_method& valve_method_pre(void);

    double& valve_costtime(void);

    Tvalve_method& valve_method_post(void);

    /** DB-API functions. */
    Tdb_method& db_method(void);

    TDtype& db_condition(void);

private:
    CHow(void) = delete;

    template<cHowVALVE::Tcontents T>
    auto get_ref_valve(void) -> typename std::add_lvalue_reference< decltype(cHowVALVE::Ttype<T>()) >::type {
        if( _type_ != TYPE_VALVE ) {
            throw std::out_of_range("Not Supported VALVE-API.");
        }

        auto contents = std::dynamic_pointer_cast<cHowVALVE>(_m_contents_);
        return contents->get<T>();
    }

    template<cHowDB::Tcontents T>
    auto get_ref_db(void) -> typename std::add_lvalue_reference< decltype(cHowDB::Ttype<T>()) >::type {
        if( _type_ != TYPE_DB ) {
            throw std::out_of_range("Not Supported DB-API.");
        }

        auto contents = std::dynamic_pointer_cast<cHowDB>(_m_contents_);
        return contents->get<T>();
    }

};



}   // principle


#endif // _PRINCIPLE_6_CLASSES_H_