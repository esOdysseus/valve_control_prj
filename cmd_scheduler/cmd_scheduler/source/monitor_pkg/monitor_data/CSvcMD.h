#ifndef _C_SERVICE_MONITORING_DATA_H_
#define _C_SERVICE_MONITORING_DATA_H_

#include <memory>
#include <string>

#include <IMD.h>

namespace monitor_pkg {

using E_MNT_Service_FLAG = enum class E_MNT_Service_FLAG {
    EMNT_FLAG_NONE = 0,
    EMNT_FLAG_SVC_AVAILABLE = 0x00000040,     // 1 bit
    EMNT_FLAG_ERR_OCCURE    = 0x00000100      // 1 bit
};

using E_MNT_Service_ERROR = enum class E_MNT_Service_ERROR {
    EMNT_ERROR_NONE = 0,
    EMNT_ERROR_STATE        = 0x0000FFFF    // 16 bit
};

/*************************************
 * Declation of CSvcMD class
 */
class CSvcMD: public IMD<std::string,               // _ID_TYPE_
                         uint32_t,                  // _FLAG_TYPE_
                         uint32_t,                  // _ERR_TYPE_
                         E_MNT_Service_FLAG,        // _ENUM_MNT_FLAG_
                         E_MNT_Service_ERROR> {     // _ENUM_MNT_ERR_
public:
    using DataType = void;

public:
    CSvcMD(IDType app_id, std::shared_ptr<DataType> &dumy [[gnu::unused]]);

    ~CSvcMD(void);

private:
    bool set_flag_extra(E_MNT_FLAG pos, FlagType value);

};


}

#endif // _C_SERVICE_MONITORING_DATA_H_