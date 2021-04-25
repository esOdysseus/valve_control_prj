#include <strings.h>

#include <logger.h>
#include <CDBGKargVctrl.h>
#include <CException.h>
#include <CVctrlCMD.h>
#include <CProcValve.h>

namespace cmd_pkg {

using TimeType = time_pkg::CTime;
using CMDType = cmd_pkg::CVctrlCMD;
constexpr const char* CDBGKargVctrl::HOW_VALUES[];
constexpr const char* CDBGKargVctrl::WHEN_TYPES[];
constexpr const char* CDBGKargVctrl::TIME_FORMAT;
constexpr const char* CDBGKargVctrl::DATE_FORMAT;

/************************************
 * Definition of Public Functions
 */
CDBGKargVctrl::CDBGKargVctrl(std::string &cmd, std::shared_ptr<std::list<std::string>> &args, bool disable_need_save)
: IDBGKarg(cmd, args, SELF_NAME) {
    try{
        clear();
        _max_latency_sec_ = proc_pkg::CProcValve::CMD_RELOADING_PERIOD;
        if ( _max_latency_sec_ == 0 ) {
            LOGERR("max_latency_sec is NULL.");
            throw CException(E_ERROR::E_ERR_INVALID_NULL_VALUE);
        }

        _who_ = get_arg(0);
        _what_ = extract_what();
        _how_ = extract_how();
        _when_type_ = extract_when_type();
        if ( _when_type_ == VALVE_WHEN_NOW ) {
            time_t latency_sec = (time_t)(atol( get_arg(4).c_str() ));
            _when_ = TimeType::get_next<time_t, time_t>(latency_sec);
            if( latency_sec >= _max_latency_sec_ ) {
                // Assumption: Server load new-cmd from a file, within 10-minute period.
                _need_save_ = true;
            }
        }
        else if( _when_type_ == VALVE_WHEN_PERIOD ) {
            _need_save_ = true;
            E_WEEK week = (E_WEEK)( atoi(get_arg(4).c_str()) );
            std::string time_s = get_arg(5);
            _when_ = TimeType::get_next<time_t>(0, week, time_s.c_str());
        }
        else if( _when_type_ == VALVE_WHEN_EVENT ) {
            double elapsed_time = -1.0;
            std::string date_s = get_arg(4);
            std::string time_s = get_arg(5);
            _when_ = TimeType::convert<time_t>(time_s.c_str(), date_s.c_str());
            elapsed_time = TimeType::elapsed_time( _when_, TimeType::get<time_t>() );

            if ( elapsed_time < -10.0 ) {   // 현재 시간보다 10초 이상 뒷처져 있다면, 문제 있음.
                LOGERR("CMD date/time(%s, %s) is old-time. please check it.", date_s.c_str(), time_s.c_str() );
                throw CException(E_ERROR::E_ERR_INVALID_VALUE);
            }
            
            if ( elapsed_time >= (double)_max_latency_sec_ ) {
                _need_save_ = true;
            }
        }

        if( disable_need_save ) {
            _need_save_ = false;
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw;
    }
}

CDBGKargVctrl::~CDBGKargVctrl(void) {
    clear();
}

bool CDBGKargVctrl::need_save_file(void) {
    return _need_save_;
}

std::string CDBGKargVctrl::cvt_event_cmdstr(void) {
    std::string cvted_cmd;
    try{
        cvted_cmd = std::string(CMD_DEBUG) + " " + get_cmd() 
                    + " " + get_who()
                    + " " + std::to_string(get_what())
                    + " " + get_how()
                    + " " + std::string(VALVE_WHEN_EVENT)
                    + " " + TimeType::print(get_when(), DATE_FORMAT)
                    + " " + TimeType::print(get_when(), TIME_FORMAT);
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
        throw CException(E_ERROR::E_ERR_FAIL_CONVERTING_TIME);
    }
    return cvted_cmd;
}

/************************************
 * Definition of Private Functions
 */
inline void CDBGKargVctrl::clear(void) {
    _who_.clear();
    _what_ = CMDType::E_TARGET::E_VALVE_NULL;
    _how_.clear();
    _when_type_.clear();
    bzero(&_when_, sizeof(_when_));
    _need_save_ = false;
    _max_latency_sec_ = 0;
}

bool CDBGKargVctrl::validation_check(std::string &str, const char* const candidates[]) const {
    assert(candidates != NULL);

    try {
        int i=0;
        for(; candidates[i] != NULL; i++) {
            if( str == candidates[i] )
                break;
        }

        if( candidates[i] != NULL )
            return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }
    return false;
}

CDBGKargVctrl::E_TARGET CDBGKargVctrl::extract_what(void) {
    E_TARGET target = E_TARGET::E_VALVE_NULL;
    target = (E_TARGET)( atoi(get_arg(1).c_str()) );
    assert( E_TARGET::E_VALVE_LEFT_01 <= target && target <= E_TARGET::E_VALVE_LEFT_04);
    return target;
}

std::string CDBGKargVctrl::extract_how(void) {
    std::string str = get_arg(2);
    assert( validation_check(str, HOW_VALUES) == true);
    return str;
}

std::string CDBGKargVctrl::extract_when_type(void) {
    std::string str = get_arg(3);
    assert( validation_check(str, WHEN_TYPES) == true);
    return str;
}


}