

#include <logger.h>
#include <IMonitor.h>
#include <CPacketMD.h>
#include <CSvcMD.h>
#include <CException.h>

namespace monitor_pkg {

template class IMonitor<CPacketMD>;
template class IMonitor<CSvcMD>;

static bool apply_double2utc(double &time, struct timespec &target);

/***********************************
 * Definition of Public Function.
 */
template <typename MONITORING_DATA>
IMonitor<MONITORING_DATA>::IMonitor(void) {
    clear();
}

template <typename MONITORING_DATA>
IMonitor<MONITORING_DATA>::~IMonitor(void) {
    clear();
}

template <typename MONITORING_DATA>
ssize_t IMonitor<MONITORING_DATA>::size(void) {
    return _data_.size();
}

template <typename MONITORING_DATA>
bool IMonitor<MONITORING_DATA>::is_there(IDType id) {
    try {
        std::shared_lock<std::shared_mutex> lock(_mtx_data_);
        auto itor = _data_.find(id);

        if ( itor != _data_.end() ) {
            return true;
        }
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
    }

    return false;
}

template <typename MONITORING_DATA>
bool IMonitor<MONITORING_DATA>::insert(IDType id, std::shared_ptr<DataType> &data) {
    std::shared_ptr<MONITORING_DATA> new_data;
    
    try {
        // Create new data.
        new_data = std::make_shared<MONITORING_DATA>(id, data);
        assert(new_data.get() != NULL);

        // insert new data to map
        std::lock_guard<std::shared_mutex> lock(_mtx_data_);
        auto res = _data_.insert({id, nullptr});
        if ( res.second ) {
            res.first->second = new_data;
        }
        else {
            LOGW("Already exist about element of key-id");
            res.first->second = new_data;
        }
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw CException(E_ERROR::E_ERR_FAIL_INSERT_DATA);
    }

    return false;
}

template <typename MONITORING_DATA>
bool IMonitor<MONITORING_DATA>::update(IDType id, E_MNT_FLAG pos, FlagType value) {
    try {
        std::lock_guard<std::shared_mutex> lock(_mtx_data_);
        auto itor = _data_.find(id);

        if ( itor != _data_.end() ) {
            itor->second->set_flag(pos, value);
            return true;
        }
        return false;
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
    }

    return false;
}

template <typename MONITORING_DATA>
bool IMonitor<MONITORING_DATA>::update(IDType id, E_MNT_ERROR pos, ErrType value) {
    try {
        std::lock_guard<std::shared_mutex> lock(_mtx_data_);
        auto itor = _data_.find(id);

        if ( itor != _data_.end() ) {
            itor->second->set_err(pos, value);
            return true;
        }
        return false;
    }
    catch (const std::exception &e) {
        LOGERR("%s", e.what());
    }

    return false;
}

template <typename MONITORING_DATA>
bool IMonitor<MONITORING_DATA>::remove(IDType id) {
    try {
        std::lock_guard<std::shared_mutex> lock(_mtx_data_);
        auto itor = _data_.find(id);

        if ( itor != _data_.end() ) {
            _data_.erase(itor);
        }
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}

template <typename MONITORING_DATA>
typename IMonitor<MONITORING_DATA>::FlagType IMonitor<MONITORING_DATA>::get(IDType id, E_MNT_FLAG pos) {
    FlagType res = 0;

    try {
        std::shared_lock<std::shared_mutex> lock(_mtx_data_);
        auto itor = _data_.find(id);

        if ( itor != _data_.end() ) {
            res = itor->second->get_flag(pos);
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return res;
}

template <typename MONITORING_DATA>
typename IMonitor<MONITORING_DATA>::ErrType IMonitor<MONITORING_DATA>::get(IDType id, E_MNT_ERROR pos) {
    ErrType res = 0;

    try {
        std::shared_lock<std::shared_mutex> lock(_mtx_data_);
        auto itor = _data_.find(id);

        if ( itor != _data_.end() ) {
            res = itor->second->get_err(pos);
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return res;
}

template <typename MONITORING_DATA>
struct timespec IMonitor<MONITORING_DATA>::get_time(IDType id) {
    double time = -1.0;
    struct timespec res;

    try 
    {
        {
            std::shared_lock<std::shared_mutex> lock(_mtx_data_);
            auto itor = _data_.find(id);

            if ( itor != _data_.end() ) {
                time = itor->second->get_time();
            }
            else {
                LOGW("There is invalid time-value(%f)", time);
                throw CException(E_ERROR::E_ERR_NOT_HAVE_MEMBER);
            }
        }

        if( time > 0.0 ) {
            if( apply_double2utc(time, res) == false ) {
                LOGERR("Converting double to timespec struct is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_CONVERTING_TIME);
            }
        }
        else {
            LOGW("There is invalid time-value(%f)", time);
            throw CException(E_ERROR::E_ERR_INVALID_VALUE);
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }

    return res;
}

template <typename MONITORING_DATA>
double IMonitor<MONITORING_DATA>::get_elapsed_time(IDType id) {
    double res = -1.0;

    try {
        std::shared_lock<std::shared_mutex> lock(_mtx_data_);
        auto itor = _data_.find(id);

        if ( itor != _data_.end() ) {
            res = itor->second->get_elapsed_time();
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return res;
}

/***********************************
 * Definition of Private Function.
 */
template <typename MONITORING_DATA>
void IMonitor<MONITORING_DATA>::clear(void) {
    std::lock_guard<std::shared_mutex> lock(_mtx_data_);
    _data_.clear();
}


/************************************
 * Definition of Local-Function.
 */
static bool apply_double2utc(double &time, struct timespec &target) {
    try {
        target.tv_sec = (long)time;
        target.tv_nsec = (time - (long)time) * 1000000000L;
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("apply_double2utc is failed." );
    }
    return false;
}


}   // namespace monitor_pkg
