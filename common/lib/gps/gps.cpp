#include <stdlib.h>
#include <gps.h>
#include <time_kes.h>

#include <logger.h>

namespace gps_pkg {

constexpr const char* Cgps::NMEA0183_PREFIX;


/*******************************
 * Public Function Definition.
 */
Cgps::Cgps( const char* UART_PATH, Tbr baud_rate )
: IUart( _m_is_continue_ ) {
    try {
        clear();

#ifdef TEST_MODE_GPS_ENABLE
        _m_state_ = TState::E_STATE_ACTIVE;
#else

        if( UART_PATH == NULL ) {
            LOGI("UART_PATH is NULL. getenv(EXPORT_ENV_GPS_PATH)");

            UART_PATH = getenv("EXPORT_ENV_GPS_PATH");
            if ( UART_PATH == NULL ) {
                LOGI("Disable Cgps class.");
                return ;
            }
        }

        if( baud_rate == Tbr::E_BR_NONE ) {
            throw std::runtime_error("Baud Rate is None.");
        }

        init( UART_PATH, baud_rate );
        create_threads();
#endif
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

Cgps::~Cgps(void) {
    destroy_threads();
    clear();
}

void Cgps::reset(void) {
    {
        std::unique_lock<std::mutex> lk(_mtx_time_);
        _m_time_.reset();
    }

    {
        std::unique_lock<std::mutex> lk(_mtx_gps_);
        _m_gps_.reset();
    }
}

bool Cgps::is_active(void) {
    if( _m_state_ == TState::E_STATE_ACTIVE ) {
        return true;
    }
    LOGW("GPS-Module is in-active state.");
    return false;
}

double Cgps::get_time(void) {
    double now = 0.0;
    double time = 0.0;
    try {
        if( is_active() == false ) {
            return time;
        }

#ifdef TEST_MODE_GPS_ENABLE
        time = ::time_pkg::CTime::get<double>();    // temporary code.
#else

        std::unique_lock<std::mutex> lk(_mtx_time_);
        if ( (_m_time_.get() == NULL) && (true == _m_is_continue_.load()) ) {
            // block until receive next-gps data. ( use conditional_variable )
            _mcv_time_.wait(lk, [&]() {
                return ((_m_time_.get() != NULL) || (false == _m_is_continue_.load()));
            });
        }

        if( _m_is_continue_ == false ) {
            return time;
        }

        // try to get GPS-time
        now = ::time_pkg::CTime::get<double>();
        time = _m_time_->time_gps + (now - _m_time_->time_sys);
#endif
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw ;
    }

    return time;
}

std::shared_ptr<Cgps::Gps> Cgps::get_gps(void) {
    std::shared_ptr<Gps> temp;
    try {
        if( is_active() == false ) {
            return temp;
        }

#ifdef TEST_MODE_GPS_ENABLE
        temp = std::make_shared<Gps>();
        if( temp.get() == NULL ) {
            throw std::runtime_error("Can not allocate memory for Test_Mode GPS-data.");
        }
        temp->time_sys = ::time_pkg::CTime::get<double>();  // temporary code.
        temp->time_gps = temp->time_sys;
        temp->latitude = 37.487935;         // for Seoul
        temp->longitude = 126.857758;       // for Seoul
        temp->spd_kmh = 0.0;
#else

        std::unique_lock<std::mutex> lk(_mtx_gps_);
        if ( (_m_gps_.get() == NULL) && (true == _m_is_continue_.load()) ) {
            // block until receive next-gps data. ( use conditional_variable )
            _mcv_gps_.wait(lk, [&]() {
                return ((_m_gps_.get() != NULL) || (false == _m_is_continue_.load()));
            });
        }

        if( _m_is_continue_ == false ) {
            return temp;
        }

        temp = std::make_shared<Gps>(*_m_gps_);
        if( temp.get() == NULL ) {
            throw std::runtime_error("Can not copy for GPS-data.");
        }
#endif
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw ;
    }

    return temp;
}


/*******************************
 * Private Function Definition.
 */
void Cgps::clear(void) {
    _m_state_ = TState::E_STATE_INACTIVE;
    _m_is_continue_ = false;
    reset();
}

void Cgps::set_time( std::shared_ptr<Gps> gps ) {
    try {
        if( gps.get() == NULL ) {
            throw std::invalid_argument("gps-time is NULL.");
        }

        if( gps->time_gps <= 0.0 || gps->time_sys <= 0.0 ) {
            throw std::invalid_argument("gps-time is invalid-data.");
        }

        {
            std::unique_lock<std::mutex> lk(_mtx_time_);
            _m_time_.reset();
            _m_time_ = gps;
        }
        _mcv_time_.notify_all();
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void Cgps::set_gps( std::shared_ptr<Gps> gps ) {
    try {
        if( gps.get() == NULL ) {
            throw std::invalid_argument("gps-data is NULL.");
        }

        if( gps->check_validation() == false ) {
            LOGW("gps-data is invalid-data.");
            return ;
        }

        {
            std::unique_lock<std::mutex> lk(_mtx_gps_);
            _m_gps_.reset();
            _m_gps_ = gps;
        }
        _mcv_gps_.notify_all();
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

bool Cgps::parse_data( std::vector<uint8_t>& data, double sys_time, std::shared_ptr<Gps>& gps ) {
    bool result = false;
    LOGI("Parse GPS-data.");

    try {
        if( data.size() <= 0 || sys_time <= 0.0 ) {
            std::string err = "Input-data is invalid-values.(size:" + std::to_string(data.size()) + ", time:" + std::to_string(sys_time) + ")";
            throw std::invalid_argument(err);
        }

        std::string msg(data.begin(), data.end());
        // msg = "$GNRMC,074910.00,A,2235.51781,N,11353.51624,E,0.008,,231216,,,D*60";     // TODO temporary Code
        LOGI("msg=%s", msg.c_str());

        // parse NMEA-0183 protocol.
        gps.reset();
        gps = parse_NMEA0183(msg);
        if( gps.get() != NULL && gps->time_gps > 0.0 ) {
            gps->time_sys = sys_time;
            _m_state_ = TState::E_STATE_ACTIVE;
            result = true;
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}

std::shared_ptr<Cgps::Gps> Cgps::parse_NMEA0183( std::string& msg ) {
    std::shared_ptr<Cgps::Gps> result;
    try {
        std::string time;
        double d_time = 0.0;
        std::vector<std::string> contents;
        size_t leng = sizeof(NMEA0183_PREFIX);
        size_t idx = msg.find(NMEA0183_PREFIX);
        if( idx == std::string::npos ) {
            LOGW("It's not NMEA0183 Protocol.");
            return result;
        }

        do {    // Split contents according with delimiter ','
            idx = msg.find(',',leng);

            if( idx == std::string::npos ) {
                LOGW("End of NMEA0183 msg.");
                contents.push_back(msg.substr(leng, std::string::npos));
            } else if( idx-leng == 0 ) {
                contents.push_back(std::string());
            } else {
                contents.push_back(msg.substr(leng, idx-leng));
            }

            leng = idx + 1;
        } while( idx != std::string::npos );


        // Check whether GPS-time is NULL.
        if( contents[0].empty() == true || contents[0].size() == 0 ) {
            LOGW("UTC-time is null value.");
            return result;
        }

        // Allocate memory for GPS-structure.
        result = std::make_shared<Gps>();
        if( result.get() == NULL ) {
            throw std::runtime_error("Can not allocate memory to GPS.");
        }

        // Set GPS-time values.
        time = contents[0].substr(0,6);                     // We only need HHMMSS
        d_time = std::stod(time);
        result->time_gps = ::time_pkg::CTime::convert<double>(time, contents[8].data(),"%H%M%S", "%d%m%y");
        result->time_gps += (9.0 * 3600.0);                 // Append 9 hour for Korean-Time.
        result->time_gps += (d_time - ((int)(d_time)*1.0)); // Append milisecond
    

        // Check GPS-data validation
        if( contents[1] != "A" ) {
            LOGW("It's not invalid GPS-data.");
            return result;
        }

        // Set GPS-data.
        result->latitude = std::stod( contents[2] );            // 위도
        result->longitude= std::stod( contents[4] );            // 경도
        result->spd_kmh = std::stod( contents[6] ) * 1.852;     // speed (unit: km/h)
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return result;
}


/** Thread related function Definition */
void Cgps::create_threads(void) {
    try {
        if( _m_is_continue_.exchange(true) == false ) {
            LOGI("Create GPS-receiving thread.");
            _m_gps_receiver_ = std::thread(&Cgps::handle_gps_rx, this);

            if ( _m_gps_receiver_.joinable() == false ) {
                _m_is_continue_ = false;
            }
        }

        if( _m_is_continue_ == false ) {
            destroy_threads();
            throw std::runtime_error("Creating GPS-receiving threads is failed.");
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void Cgps::destroy_threads(void) {
    if( _m_is_continue_.exchange(false) == true ) {
        if( _m_gps_receiver_.joinable() == true ) {
            LOGI("Destroy GPS-receiving thread.");
            _mcv_time_.notify_all();
            _mcv_gps_.notify_all();
            _m_gps_receiver_.join();
        }
    }
}

void Cgps::handle_gps_rx(void) {
    // TState state = TState::E_STATE_ERROR;
    double sys_time = 0.0;
    std::shared_ptr<Gps> gps;
    std::vector<uint8_t> buffer;
    buffer.reserve( 2* READ_BUF_SIZE );

    LOGI("Run GPS-receiver thread.");
    _m_state_ = TState::E_STATE_INACTIVE;

    while( _m_is_continue_ == true ) {
        try {
            // Get GPS-data from Uart GPS-Module 
            read_data(_m_fd_, buffer);      // Blocking API
            sys_time = ::time_pkg::CTime::get<double>();

            // Parsing GPS-data
            if( parse_data(buffer, sys_time, gps) == true ) {
                // Set GPS & notify
                set_time( gps );
                set_gps( gps );
            }
        }
        catch ( const std::exception& e ) {
            LOGERR("%s", e.what());
        }
    }

    _m_state_ = TState::E_STATE_INACTIVE;
    LOGI("Exit GPS-receiver thread.");
}


}   // gps_pkg