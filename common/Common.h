#ifndef _COMMON_DEFINITION_H_
#define _COMMON_DEFINITION_H_

#include <mutex>
#include <string>
#include <iostream>

namespace common {

/*******************************
 * Command(CMD)-format
 ******************************/
typedef enum E_FLAG {
    E_FLAG_NONE = 0,
    E_FLAG_REQUIRE_RESP = 0x01,     // 0: not-require RESP      , 1: require RESP (ACT-DONE or RESP)
    E_FLAG_REQUIRE_ACK  = 0x02,     // 0: not-require ACK       , 1: require ACK
    E_FLAG_REQUIRE_ACT  = 0x04,     // 0: not-require ACT-START , 1: require ACT-START
    E_FLAG_KEEPALIVE    = 0x08,     // 0: not keep-alive msg    , 1: keep-alive msg
    E_FLAG_RESP_MSG     = 0x10,     // 0: REQ/PUB message       , 1: RESP message
    E_FLAG_ACK_MSG      = 0x20,     // 0: not ack message       , 1: ACK message
    E_FLAG_ACTION_START = 0x40,     // 0: not act-start msg     , 1: ACT-START msg
    E_FLAG_STATE_ERROR  = 0x80,     // 0: normal state          , 1: abnormal state (detail is depend on E_STATE)
    E_FLAG_ALL          = 0xFF
} E_FLAG;

typedef enum E_STATE {
    E_NO_STATE              = 0x0000,
    E_STATE_THR_GPS         = 0x0001,   // GP-ctrl  -Thread: It indicate place that CMD is triggered.
    E_STATE_THR_CMD         = 0x0002,   // Service  -Thread: It indicate place that CMD is triggered.
    E_STATE_THR_KEEPALIVE   = 0x0004,   // KeepAlive-Thread: It indicate place that CMD is triggered.
    E_STATE_OUT_OF_SERVICE  = 0x0008,   // If Service is stoped, then this state set.
    E_STATE_OCCURE_ERROR    = 0x0010,   // If Unintended-System Error is occured, then this state set.
    E_STATE_ACTION_FAIL     = 0x0020,   // 0: not exist means  , 1: fail with action
    E_STATE_ALL             = 0xFFFF
} E_STATE;

typedef uint16_t    StateType;

}   // common


namespace alias {

    class CAlias {
    public:
        std::string app_path;
        std::string pvd_id;

    public:
        CAlias( const CAlias& myself ) {
            clear();
            set( myself.app_path, myself.pvd_id );
        }

        CAlias( CAlias&& myself ) {
            clear();
            app_path = std::move(myself.app_path);
            pvd_id = std::move(myself.pvd_id);
        }

        CAlias(const std::string pvd_full_path) {
            std::string app;
            std::string pvd;
            
            clear();
            extract_app_pvd(pvd_full_path, app, pvd);
            set(app, pvd);
        }

        CAlias(std::string app, std::string pvd) {
            clear();
            set(app, pvd);
        }

        ~CAlias(void) {
            clear();
        }
        
        CAlias& operator=(const CAlias& myself) {
            clear();
            set(myself.app_path, myself.pvd_id);
        }

        bool empty(void) {
            return pvd_id.empty();
        }

        // getter
        common::StateType get_state(common::E_STATE pos) {
            if( pos == common::E_STATE::E_NO_STATE) {
                throw std::invalid_argument("pos is E_NO_STATE.");
            }

            std::lock_guard<std::mutex>  guard(_mtx_state_);
            return _m_state_ & pos;
        }

        // setter
        void set_state(common::E_STATE pos, common::StateType value) {
            int shift_cnt = 0;

            if ( pos == common::E_STATE::E_NO_STATE ) {
                std::lock_guard<std::mutex>  guard(_mtx_state_);
                _m_state_ = value;
            }
            else {
                // Assumption : pos is continuous-bitmask.
                while( ((1<<shift_cnt) & pos) == 0 ) {
                    shift_cnt++;
                    if( shift_cnt >= (sizeof(common::StateType)*8) ) {
                        throw std::logic_error("shift_cnt is overflowed.");
                    }
                }

                std::lock_guard<std::mutex>  guard(_mtx_state_);
                _m_state_ = (_m_state_ & (~pos)) | ((value << shift_cnt) & pos);
            }
        }

    private:
        void clear(void) {
            app_path.clear();
            pvd_id.clear();
            _m_state_ = common::E_STATE::E_NO_STATE;
        }

        void set( std::string app, std::string pvd ) {
            app_path = app;
            pvd_id = pvd;
        }

        static void extract_app_pvd(const std::string& full_path, std::string& app, std::string& pvd) {
            std::string delimiter = "/";
            size_t pos = 0;

            try {
                pos = full_path.rfind(delimiter);
                if( pos == std::string::npos ) {
                    std::string err = "full_path(" + full_path + ") is invalid.";
                    throw std::invalid_argument(err);
                }

                app = full_path.substr(0, pos);
                pvd = full_path.substr(pos+delimiter.length(), full_path.length());
            }
            catch( const std::exception& e ) {
                std::cout << "[Error] CAlias::extract_app_pvd: " << e.what() << std::endl;
                throw e;
            }
        }

    private:
        common::StateType   _m_state_;

        std::mutex _mtx_state_;

    };


}   // alias



#endif // _COMMON_DEFINITION_H_