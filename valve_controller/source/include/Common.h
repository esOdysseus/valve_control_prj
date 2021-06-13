#ifndef _COMMON_DEFINITION_H_
#define _COMMON_DEFINITION_H_

#include <string>

namespace alias {

    class CAlias {
    public:
        std::string app_path;
        std::string pvd_id;

    public:
        CAlias( const CAlias& myself ) {
            set( myself.app_path, myself.pvd_id );
        }

        CAlias( CAlias&& myself ) {
            app_path = std::move(myself.app_path);
            pvd_id = std::move(myself.pvd_id);
        }

        CAlias(std::string app, std::string pvd) {
            set(app, pvd);
        }

        ~CAlias(void) {
            clear();
        }
        
        CAlias& operator=(const CAlias& myself) {
            set(myself.app_path, myself.pvd_id);
        }

        bool empty(void) {
            return pvd_id.empty();
        }

        void set( std::string app, std::string pvd ) {
            app_path = app;
            pvd_id = pvd;
        }

        void clear(void) {
            app_path.clear();
            pvd_id.clear();
        }
    };

}   // alias


namespace valve_pkg {

typedef enum E_STATE {
    E_NO_STATE              = 0x0000,
    E_STATE_THR_GPS         = 0x0001,
    E_STATE_THR_CMD         = 0x0002,
    E_STATE_THR_KEEPALIVE   = 0x0004,
    E_STATE_OUT_OF_SERVICE  = 0x0008,
    E_STATE_OCCURE_ERROR    = 0x0010,
    E_STATE_ALL             = 0xFFFF
} E_STATE;

typedef uint16_t    StateType;

}   // valve_pkg


#endif // _COMMON_DEFINITION_H_