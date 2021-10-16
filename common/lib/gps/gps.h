#ifndef _H_CLASS_GPS_LIBRARY_
#define _H_CLASS_GPS_LIBRARY_

#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <vector>

#include <uart.h>

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
  */
namespace gps_pkg {


class Cgps: public uart::IUart {
public:
    class Gps {
    public:
        double time_sys;    // System-time when gps-time is updated by GPS-module.
        double time_gps;    // Latest gps-time that is updated by GPS-module.
        double latitude;    // 위도
        double longitude;   // 경도
        // double altitude;    // 고도
        double spd_kmh;

        Gps(void) {
            time_sys = 0.0;
            time_gps = 0.0;
            latitude = 0.0;
            longitude = 0.0;
            // altitude = 0.0;
            spd_kmh = 0.0;
        }
        ~Gps(void) = default;

        bool check_validation(void) {
            if( time_sys == 0.0 || time_gps == 0.0 || latitude == 0.0 || longitude == 0.0 ) {
                return false;
            }
            return true;
        }
    };

public:
    Cgps( const char* UART_PATH, Tbr baud_rate=Tbr::E_BR_9600 );

    ~Cgps(void);

    void reset(void);

    bool is_active(void);

    /** Get GPS-Time */
    double get_time(void);

    /** Get GPS-Position */
    std::shared_ptr<Gps> get_gps(void);

private:
    Cgps(void) = delete;

    void clear(void);

    void set_gps( std::shared_ptr<Gps> gps );

    bool parse_data( std::vector<uint8_t>& data, double sys_time, std::shared_ptr<Gps>& gps );

    std::shared_ptr<Gps> parse_NMEA0183( std::string& msg );

    /** Thread related function */
    void create_threads(void);

    void destroy_threads(void);

    void handle_gps_rx(void);

private:
    TState _m_state_;               // State of GPS-module. [ In-Active, Active ]

    /** GPS-result */
    std::mutex _mtx_gps_;
    std::condition_variable _mcv_gps_;
    std::shared_ptr<Gps> _m_gps_;   // latest GPS result

    /** GPS-receiving thread */
    std::atomic<bool> _m_is_continue_;
    std::thread _m_gps_receiver_;

    static constexpr const char* NMEA0183_PREFIX = "$GNRMC,";

};


}   // gps_pkg


#endif // _H_CLASS_GPS_LIBRARY_