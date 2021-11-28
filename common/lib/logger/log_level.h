#ifndef _H_LOG_LEVEL_DEFINITION_
#define _H_LOG_LEVEL_DEFINITION_

#include <libgen.h>     // for basename

#define LOG_LEVEL_NONE          0
#define LOG_LEVEL_ERR           1
#define LOG_LEVEL_WARN          2
#define LOG_LEVEL_INFO          3
#define LOG_LEVEL_DEBUG         4

// Configuration-setting
#ifndef LOG_LEVEL
    #define LOG_LEVEL              LOG_LEVEL_INFO
#endif

// Debug Logger mode setting.
#ifdef LOG_DEBUG_MODE
    #define LOG_EXIT(val)       // exit(val)
    #define _FUNC_NAME_         __PRETTY_FUNCTION__
    #define _FILE_NAME_         basename(__FILE__)
#else
    #define LOG_EXIT(val)
    #define _FUNC_NAME_         __FUNCTION__
    #define _FILE_NAME_         basename(__FILE__)
#endif



#endif // _H_LOG_LEVEL_DEFINITION_