/*
 * logger.h
 * Created on: Apr 21, 2015
 */
#ifndef LOGGER_KES_H_
#define LOGGER_KES_H_

#ifndef LOGGER_TAG
    #define LOGGER_TAG              "NONE"
#endif

#include <log_level.h>


#ifdef LOG_MODE_STDOUT
    #include "stdout/logger.h"
#else
    #ifdef __cplusplus
        #include "dlt/logger.h"
    #else
        #include "stdout/logger.h"
    #endif
#endif // LOG_MODE_STDOUT


// API of Logger.
#if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    #define LOGHEX(fmt, buf, length, arg...)       _PRINT_HEX_(buf, length, fmt, ##arg)
    #define LOGD(fmt, arg...)              _PRINT_D_(fmt, ##arg)
    #define LOGW(fmt, arg...)              _PRINT_W_(fmt, ##arg)
    #define LOGERR(fmt, arg...)            _PRINT_ERR_(fmt, ##arg)
    #define LOGI(fmt, arg...)              _PRINT_I_(fmt, ##arg)
#elif (LOG_LEVEL == LOG_LEVEL_INFO)
    #define LOGHEX(fmt, buf, length, arg...)        _PRINT_HEX_(buf, length, fmt, ##arg)
    #define LOGD(fmt, arg...)
    #define LOGW(fmt, arg...)              _PRINT_W_(fmt, ##arg)
    #define LOGERR(fmt, arg...)            _PRINT_ERR_(fmt, ##arg)
    #define LOGI(fmt, arg...)              _PRINT_I_(fmt, ##arg)
#elif (LOG_LEVEL == LOG_LEVEL_WARN)
    #define LOGHEX(fmt, buf, length, arg...)
    #define LOGD(fmt, arg...)
    #define LOGW(fmt, arg...)              _PRINT_W_(fmt, ##arg)
    #define LOGERR(fmt, arg...)            _PRINT_ERR_(fmt, ##arg)
    #define LOGI(fmt, arg...)
#elif (LOG_LEVEL == LOG_LEVEL_ERR)
    #define LOGHEX(fmt, buf, length, arg...)
    #define LOGD(fmt, arg...)
    #define LOGW(fmt, arg...)
    #define LOGERR(fmt, arg...)            _PRINT_ERR_(fmt, ##arg)
    #define LOGI(fmt, arg...)
#endif


#endif /* LOGGER_KES_H_ */
