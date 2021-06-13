#include <cassert>

#include <logger.h>
#include <CSvcMonitor.h>


namespace monitor_pkg {


CSvcMonitor::CSvcMonitor(void): IMonitor() {}

void CSvcMonitor::process_with_elapse_time(FuncType lamda_read_only) {
    IDType id;
    double elapsed_time = -1.0;
    MapType::iterator itor;

    try {
        std::shared_lock<std::shared_mutex> lock(_mtx_data_);
        for(itor = _data_.begin(); itor != _data_.end(); itor++){
            id = itor->first;
            elapsed_time = itor->second->get_elapsed_time();

            if( lamda_read_only(id, elapsed_time) != true) {
                LOGERR("lamda function is failed.");
                throw CException(E_ERROR::E_ERR_FAIL_EXECUTE_FUNC);
            }
        }
    }
    catch( const std::exception &e ) {
        LOGERR("%s", e.what());
        throw dynamic_cast<const CException&>(e);
    }
}


}