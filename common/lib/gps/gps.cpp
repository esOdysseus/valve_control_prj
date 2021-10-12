#include <gps.h>
#include <time_kes.h>

#include <logger.h>

namespace gps_pkg {


/*******************************
 * Public Function Definition.
 */
Cgps::Cgps( const char* UART_PATH, Tbr baud_rate )
: IUart( _m_is_continue_ ) {
    try {
        if( UART_PATH == NULL ) {
            throw std::runtime_error("UART_PATH is NULL.");
        }

        if( baud_rate == Tbr::E_BR_NONE ) {
            throw std::runtime_error("Baud Rate is None.");
        }

        clear();
        init( UART_PATH, baud_rate );
        create_threads();

#ifdef TEST_MODE_ENABLE
        _m_state_ = TState::E_STATE_ACTIVE;
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
    std::unique_lock<std::mutex> lk(_mtx_gps_);
    _m_gps_.reset();
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

    if( is_active() == false ) {
        return time;
    }

#ifdef TEST_MODE_ENABLE
    time = ::time_pkg::CTime::get<double>();    // temporary code.
#else
    std::unique_lock<std::mutex> lk(_mtx_gps_);
    if ( (_m_gps_.get() == NULL) && (true == _m_is_continue_.load()) ) {
        // block until receive next-gps data. ( use conditional_variable )
        _mcv_gps_.wait(lk, [&]() {
            return ((_m_gps_.get() != NULL) || (false == _m_is_continue_.load()));
        });
    }

    if( _m_is_continue_ == false ) {
        return time;
    }

    // try to get GPS-time
    now = ::time_pkg::CTime::get<double>();
    time = _m_gps_->time_gps + (now - _m_gps_->time_sys);
#endif
    return time;
}

std::shared_ptr<Cgps::Gps> Cgps::get_gps(void) {
    std::shared_ptr<Gps> temp;

    if( is_active() == false ) {
        return temp;
    }

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
    return temp;
}


/*******************************
 * Private Function Definition.
 */
void Cgps::clear(void) {
    _m_state_ = TState::E_STATE_INACTIVE;

    _m_is_continue_ = false;
    std::unique_lock<std::mutex> lk(_mtx_gps_);
    _m_gps_.reset();
}

void Cgps::set_gps( std::shared_ptr<Gps> gps ) {
    try {
        if( gps.get() == NULL ) {
            throw std::invalid_argument("gps input-data is NULL.");
        }

        if( gps->check_validation() == false ) {
            throw std::invalid_argument("gps input-data is invalid-data.");
        }

        std::unique_lock<std::mutex> lk(_mtx_gps_);
        _m_gps_.reset();
        _m_gps_ = gps;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

std::shared_ptr<Cgps::Gps> Cgps::parse_data( std::vector<uint8_t>& data, double sys_time ) {
    std::shared_ptr<Cgps::Gps> result;
    LOGI("Parse GPS-data.");

    try {
        if( data.size() <= 0 || sys_time <= 0.0 ) {
            std::string err = "Input-data is invalid-values.(size:" + std::to_string(data.size()) + ", time:" + std::to_string(sys_time) + ")";
            throw std::invalid_argument(err);
        }

        std::string msg(data.begin(), data.end());
        LOGW("msg=%s", msg.c_str());
        // throw std::logic_error("Not Implement Yet.");
        ;   // TODO 
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
    _m_state_ = TState::E_STATE_ACTIVE;

    while( _m_is_continue_ == true ) {
        try {
            gps.reset();
            // Get GPS-data from Uart GPS-Module 
            read_data(_m_fd_, buffer);      // Blocking API
            sys_time = ::time_pkg::CTime::get<double>();
            
            // Parsing GPS-data
            gps = parse_data(buffer, sys_time);

            // Set GPS & notify
            set_gps( gps );
            _mcv_gps_.notify_all();
        }
        catch ( const std::exception& e ) {
            LOGERR("%s", e.what());
        }
    }

    _m_state_ = TState::E_STATE_INACTIVE;
    LOGI("Exit GPS-receiver thread.");
}


}   // gps_pkg