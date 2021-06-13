#include <cassert>

#include <logger.h>
#include <CSvcMD.h>

namespace monitor_pkg {

using namespace std::placeholders;

/*********************************
 * Definition of Public Function.
 */
CSvcMD::CSvcMD(IDType app_id, std::shared_ptr<DataType> &dumy [[gnu::unused]])
: IMD(app_id, std::bind(&CSvcMD::set_flag_extra, this, _1, _2)) { }

CSvcMD::~CSvcMD(void) {
    _id_.clear();
}

/**********************************
 * Definition of Private Function.
 */
bool CSvcMD::set_flag_extra(E_MNT_FLAG pos, FlagType value) {
    LOGD("Called.");
    
    if ( pos == E_MNT_FLAG::EMNT_FLAG_SVC_AVAILABLE ) {
        set_time();
    }
    return true;
}


}