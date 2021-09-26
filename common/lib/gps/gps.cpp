#include <gps.h>
#include <time_kes.h>

#include <logger.h>

namespace gps_pkg {


/*******************************
 * Public Function Definition.
 */
Cgps::Cgps( const char* UART_PATH, Tbr baud_rate ) {
    clear();

    // TODO temporary code.
    _m_state_ = TState::E_STATE_ACTIVE;
}

Cgps::~Cgps(void) {
    clear();
}

void Cgps::reset(void) {
    ;
}

bool Cgps::is_active(void) {
    if( _m_state_ == TState::E_STATE_ACTIVE ) {
        return true;
    }

    LOGW("GPS-Module is in-active state.");
    return false;
}

double Cgps::get_time(void) {
    double time = 0.0;

    if( is_active() ) {
        // TODO try to get GPS-time
        LOGERR("Not Implemented yet.");

        // time = ::time_pkg::CTime::get<double>();    // TODO temporary code.
    }
    return time;
}


/*******************************
 * Private Function Definition.
 */
void Cgps::clear(void) {
    _m_state_ = TState::E_STATE_INACTIVE;
    _m_latest_gps_time_ = 0.0;
    _m_latest_sys_time_ = 0.0;
}


}   // gps_pkg