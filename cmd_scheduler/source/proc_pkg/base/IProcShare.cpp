
#include <mutex>

#include <logger.h>
#include <IProcShare.h>
#include <IProc.h>

namespace proc_pkg {


/*********************************
 * Definition of Public Function.
 */
IProcShare::IProcShare(void) {
    std::lock_guard<std::shared_mutex> lock(_mtx_proc_map_);
    _proc_map_.clear();
}

IProcShare::~IProcShare(void) {
    std::lock_guard<std::shared_mutex> lock(_mtx_proc_map_);
    _proc_map_.clear();
}

bool IProcShare::insert(IProc &instance) {
    try {
        std::string proc_name = instance.get_proc_name();

        std::lock_guard<std::shared_mutex> lock(_mtx_proc_map_);
        auto res = _proc_map_.insert({proc_name, nullptr});
        if ( res.second ) {
            res.first->second = &instance;
        }
        else {
            LOGW("Already exist about element of key=%s", proc_name.c_str());
            res.first->second = &instance;
        }
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;
}

bool IProcShare::remove(IProc &instance) {
    std::string proc_name = instance.get_proc_name();
    return remove( proc_name );
}

IProc* IProcShare::get(std::string &proc_name) {
    try {
        std::shared_lock<std::shared_mutex> lock(_mtx_proc_map_);
        auto itor = _proc_map_.find(proc_name);

        if ( itor != _proc_map_.end() ) {
            return itor->second;
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return NULL;
}


/************************************
 * Definition of Private Function.
 */
bool IProcShare::remove(std::string &proc_name) {
    try {
        std::lock_guard<std::shared_mutex> lock(_mtx_proc_map_);
        auto itor = _proc_map_.find(proc_name);

        if ( itor != _proc_map_.end() ) {
            _proc_map_.erase(itor);
        }
        return true;
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
    }

    return false;    
}


}