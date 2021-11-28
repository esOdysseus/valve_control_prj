#ifndef _H_LOCAL_PRINT_AS_STDOUT_
#define _H_LOCAL_PRINT_AS_STDOUT_


#include "CdltLogger.h"

#ifndef LOG_DLT_APP_ID
    #define LOG_DLT_APPID "none"
#endif // LOG_DLT_APP_ID

#ifndef LOG_DLT_CID
    #define LOG_DLT_CID   "none"
#endif // LOG_DLT_CID


// Raw-Logic definition of Logger.
#define _HEXOUT(buf, length, fmt, ...)   \
    ::dlt::CdltLogger::get_instance(LOG_DLT_APPID, LOG_DLT_CID).print(buf, length, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define _DBG(fmt, ...)   \
    ::dlt::CdltLogger::get_instance(LOG_DLT_APPID, LOG_DLT_CID).print(NULL, 0, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define _INFO(fmt, ...)  \
    ::dlt::CdltLogger::get_instance(LOG_DLT_APPID, LOG_DLT_CID).print(NULL, 0, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define _WARN(fmt, ...)  \
    ::dlt::CdltLogger::get_instance(LOG_DLT_APPID, LOG_DLT_CID).print(NULL, 0, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define _ERR(fmt, ...)   \
    {   \
        ::dlt::CdltLogger::get_instance(LOG_DLT_APPID, LOG_DLT_CID).print(NULL, 0, LOG_LEVEL_ERR, fmt, ##__VA_ARGS__);    \
        LOG_EXIT(-1);   \
    }



// API of Logger.
#define _PRINT_HEX_(buf, length, fmt, ...)       _HEXOUT(buf, length, "["LOGGER_TAG"]HEX: " "%s(%s:%d)(length=%u):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, length, ##__VA_ARGS__)
#define _PRINT_D_(fmt, ...)              _DBG("["LOGGER_TAG"]D: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__)
#define _PRINT_W_(fmt, ...)              _WARN("["LOGGER_TAG"]W: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__)
#define _PRINT_ERR_(fmt, ...)            _ERR("["LOGGER_TAG"]E: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__)
#define _PRINT_I_(fmt, ...)              _INFO("["LOGGER_TAG"]I: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__)




#endif // _H_LOCAL_PRINT_AS_STDOUT_