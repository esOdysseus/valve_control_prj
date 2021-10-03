#ifndef _COMMON_DEFINITION_H_
#define _COMMON_DEFINITION_H_

#include <string>
#include <shared_mutex_kes.h>


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
    /** for Time-Synchronization */
    E_STATE_TIME_SYNC       = 0x0001,   // [Local -Set] doing Time-Sync processing. (0: normal, 1: doing)
    E_STATE_TIME_ON         = 0x0002,   // [Global-Set] reliable Time, or not. (0: not reliable, 1: reliable)
    E_STATE_TIME_SRC        = 0x0004,   // [Global-Set] have Time-source(GPS/NTP), or not (0: not have, 1: have)
    /** for announcing Running-Thread */
    E_STATE_THR_CMD         = 0x0008,   // [Local -Set] Service  -Thread: It indicate place that CMD is triggered.
    E_STATE_THR_KEEPALIVE   = 0x0010,   // [Local -Set] KeepAlive-Thread: It indicate place that CMD is triggered.
    /** for announcing Service-Availability */
    E_STATE_OUT_OF_SERVICE  = 0x0020,   // [Global-Set] If Service is stoped, then this state set.
    /** for announcing System/Task Error */
    E_STATE_OCCURE_ERROR    = 0x0040,   // [Global-Set] If Unintended-System Error is occured, then this state set.
    E_STATE_ACTION_FAIL     = 0x0080,   // [Global-Set] 0: not exist means  , 1: fail with action
    
    /** for reaction-sending corresponded with TASK */
    E_STATE_REACT_ACTION_START  = 0x1000,   // [Internal-Use] It need to send ACTION-START packet to peer.
    E_STATE_REACT_ACTION_DONE   = 0x2000,   // [Internal-Use] It need to send ACTION-DONE packet to peer.
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
        CAlias( const CAlias& myself );

        CAlias( CAlias&& myself );

        CAlias(const std::string pvd_full_path, bool is_self=false);

        CAlias(std::string app, std::string pvd, bool is_self=false);

        ~CAlias(void);
        
        CAlias& operator=(const CAlias& myself);

        bool empty(void);

        // getter
        common::StateType get_state(common::E_STATE pos);

        std::string get_machine_name(void);

        // setter
        void set_state(common::E_STATE pos, common::StateType value);

        void set_machine_name( std::string name );

        std::string get_full_path(void);

    private:
        void clear(void);

        void set( std::string app, std::string pvd );

        static void extract_app_pvd(const std::string& full_path, std::string& app, std::string& pvd);

        void get_self_machine( void );

    private:
        common::StateType _m_state_;
        std::string _m_machine_;

        std::shared_mutex _mtx_state_;
        std::shared_mutex _mtx_machine_;

    };


}   // alias



#endif // _COMMON_DEFINITION_H_