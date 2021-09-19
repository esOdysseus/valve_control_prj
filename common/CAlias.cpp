#include <mutex>
#include <Common.h>

#include <logger.h>


namespace alias {


static constexpr const char* ENV_MACHINE_NAME = "MACHINE_DEVICE_NAME";

/*****************************************
 * Public Function Definition
 */
CAlias::CAlias( const CAlias& myself ) {
    try {
        clear();
        set( myself.app_path, myself.pvd_id );
        _m_state_ = myself._m_state_;
        _m_machine_ = myself._m_machine_;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

CAlias::CAlias( CAlias&& myself ) {
    try {
        clear();
        app_path = std::move(myself.app_path);
        pvd_id = std::move(myself.pvd_id);
        {
            std::lock_guard<std::shared_mutex>  guard(myself._mtx_state_);
            _m_state_ = std::move(myself._m_state_);
        }
        {
            std::lock_guard<std::shared_mutex>  guard(myself._mtx_machine_);
            _m_machine_ = std::move(myself._m_machine_);
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

CAlias::CAlias(const std::string pvd_full_path, bool is_self) {
    std::string app;
    std::string pvd;

    try {
        clear();
        extract_app_pvd(pvd_full_path, app, pvd);
        set(app, pvd);
        if( is_self == true ) {
            get_self_machine();
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

CAlias::CAlias(std::string app, std::string pvd, bool is_self) {
    try {
        clear();
        set(app, pvd);
        if( is_self == true ) {
            get_self_machine();
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

CAlias::~CAlias(void) {
    clear();
}

CAlias& CAlias::operator=(const CAlias& myself) {
    clear();
    set(myself.app_path, myself.pvd_id);
    _m_state_ = myself._m_state_;
    _m_machine_ = myself._m_machine_;
}

bool CAlias::empty(void) {
    return pvd_id.empty();
}

// getter
common::StateType CAlias::get_state(common::E_STATE pos) {
    if( pos == common::E_STATE::E_NO_STATE) {
        throw std::invalid_argument("pos is E_NO_STATE.");
    }

    std::shared_lock<std::shared_mutex>  guard(_mtx_state_);
    return _m_state_ & pos;
}

std::string CAlias::get_machine_name(void) {
    std::shared_lock<std::shared_mutex>  guard(_mtx_machine_);
    return _m_machine_;
}

// setter
void CAlias::set_state(common::E_STATE pos, common::StateType value) {
    int shift_cnt = 0;
    try {
        if ( pos == common::E_STATE::E_NO_STATE ) {
            std::lock_guard<std::shared_mutex>  guard(_mtx_state_);
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

            std::lock_guard<std::shared_mutex>  guard(_mtx_state_);
            _m_state_ = (_m_state_ & (~pos)) | ((value << shift_cnt) & pos);
        }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void CAlias::set_machine_name( std::string name ) {
    try {
        {
            std::shared_lock<std::shared_mutex>  guard(_mtx_machine_);
            if( _m_machine_ == name ) {
                return ;
            }
        }

        std::lock_guard<std::shared_mutex>  guard(_mtx_machine_);
        if( _m_machine_.empty() != true ) {
            std::string err = "MACHINE_NAME is already set with \"" + _m_machine_ 
                            + "\". So, we can't set it as new-name.(" + name + ")";
            throw std::logic_error(err);
        }

        // Only, in case that _m_machine_ is empty, you can set it.
        _m_machine_ = name;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

std::string CAlias::get_full_path(void) {
    return app_path + "/" + pvd_id;
}


/*****************************************
 * Private Function Definition
 */
void CAlias::clear(void) {
    app_path.clear();
    pvd_id.clear();
    _m_machine_.clear();
    _m_state_ = common::E_STATE::E_NO_STATE;
}

void CAlias::set( std::string app, std::string pvd ) {
    app_path = app;
    pvd_id = pvd;
}

void CAlias::extract_app_pvd(const std::string& full_path, std::string& app, std::string& pvd) {
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
        LOGERR("%s", e.what());
        throw e;
    }
}

void CAlias::get_self_machine( void ) {
    try {
        char* self_machine = getenv(ENV_MACHINE_NAME);
        if( self_machine == NULL ) {
            std::string err = std::string(ENV_MACHINE_NAME) + " is empty. (export " + ENV_MACHINE_NAME + "=XXXX)";
            throw std::invalid_argument(err);
        }

        LOGW("%s = %s", ENV_MACHINE_NAME, self_machine);
        set_machine_name( self_machine );
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}



}   // alias