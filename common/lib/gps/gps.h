#ifndef _H_CLASS_GPS_LIBRARY_
#define _H_CLASS_GPS_LIBRARY_

/******************
 * GPS library class
 * 
 *  - Objectives
 *      1. We have to get UTC time from GPS-signal.
 *      2. We have to get Latitude/Longitude values from GPS-signal.
 *      3. We need to know Status about GPS-module. (Active/InActive, Frequency)
 * 
 *  - Assumption
 *      1. GPS-module communication Port: Uart
 * 
 *  - Reference Site
 *      1. https://imsoftpro.tistory.com/48
 *      2. https://codingcoding.tistory.com/643
 */
namespace gps_pkg {


class Cgps {
public:
    using Tbr = enum class _enum_baud_rate_ {
        E_BR_NONE = 0,
        E_BR_9600,
        E_BR_115200
    };

private:
    using TState = enum class _enum_state_ {
        E_STATE_INACTIVE = 0,
        E_STATE_ACTIVE
    };

public:
    Cgps( const char* UART_PATH, Tbr baud_rate=Tbr::E_BR_NONE );

    ~Cgps(void);

    void reset(void);

    double get_time(void);

private:
    Cgps(void) = delete;

private:
    TState _m_state_;               // State of GPS-module. [ In-Active, Active ]

    double _m_latest_gps_time_;     // Latest gps-time that is updated by GPS-module.

    double _m_latest_sys_time_;     // System-time when gps-time is updated by GPS-module.

};


}   // gps_pkg


#endif // _H_CLASS_GPS_LIBRARY_