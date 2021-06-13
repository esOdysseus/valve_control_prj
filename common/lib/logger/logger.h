/*
 * logger.h
 * Created on: Apr 21, 2015
 */
#ifndef LOGGER_KES_H_
#define LOGGER_KES_H_

#include <stdio.h>
#include <libgen.h>

#ifdef LOG_TIME_ENABLE
    #include <assert.h>
    #include <time.h>
#endif

#ifndef LOGGER_TAG
    #define LOGGER_TAG              "NONE"
#endif
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

// Color-Setting of Text.
#define LOG_COLOR_RED           "\033[0;31m"
#define LOG_COLOR_BROWN         "\033[0;33m"
#define LOG_COLOR_BLUE          "\033[0;34m"
#define LOG_COLOR_END           "\033[0;m"

// Get Current-Time
#ifdef LOG_TIME_ENABLE
    #define TIME_BUF_SIZE       24
    #define GET_CUR_TIME(_VAR_,_SIZE_) \
    {	\
        assert(_SIZE_>=24);	\
        /* print current-time for human-readable. */ \
        struct timespec __tspec__;  \
        tm __cur_tm__;              \
        \
        /* get current-time by REAL-TIME-CLOCK. */  \
        assert(clock_gettime(CLOCK_REALTIME, &__tspec__) != -1);	\
        \
        /* timespec 구조체의 초수 필드(tv_sec)를 tm 구조체로 변환 */    \
        localtime_r((time_t *)&__tspec__.tv_sec, &__cur_tm__);          \
        strftime((_VAR_), (_SIZE_), "[%Y-%m-%d][%T]", &__cur_tm__);	\
    }
#endif

// Raw-Logic definition of Logger.
#ifdef LOG_TIME_ENABLE
    #define _HEXOUT(fmt, buf, length)   \
            {               \
                int _i_ = 0;      \
                printf("["LOGGER_TAG"]HEX: " "%s(%s:%d):length=%u, HEXOUTPUT:\n" fmt, _FUNC_NAME_, _FILE_NAME_, __LINE__, length);   \
                for(_i_=0; _i_ < (length); _i_++)       \
                {           \
                    printf(" %02X", (buf)[_i_]);   \
                }           \
                printf( "\n");   \
            }
    #define _DBG(fmt, arg...)   \
            {   \
                char __time_str__[TIME_BUF_SIZE]="";  \
                GET_CUR_TIME(__time_str__, TIME_BUF_SIZE); \
                printf("%s""["LOGGER_TAG"]D: " "%s(%s:%d):" fmt "\n", __time_str__, _FUNC_NAME_, _FILE_NAME_, __LINE__, ##arg); \
            }
    #define _INFO(fmt, arg...)  \
            {   \
                char __time_str__[TIME_BUF_SIZE]="";  \
                GET_CUR_TIME(__time_str__, TIME_BUF_SIZE); \
                printf("%s""["LOGGER_TAG"]I: " "%s(%s:%d):" fmt "\n", __time_str__, _FUNC_NAME_, _FILE_NAME_, __LINE__, ##arg); \
            }
    #define _WARN(fmt, arg...)  \
            {   \
                char __time_str__[TIME_BUF_SIZE]="";  \
                GET_CUR_TIME(__time_str__, TIME_BUF_SIZE); \
                printf("%s""["LOGGER_TAG"]W: " "%s(%s:%d):" fmt "\n", __time_str__, _FUNC_NAME_, _FILE_NAME_, __LINE__, ##arg); \
            }
    #define _ERR(fmt, arg...)   \
            {   \
                char __time_str__[TIME_BUF_SIZE]="";  \
                GET_CUR_TIME(__time_str__, TIME_BUF_SIZE); \
                printf("%s""["LOGGER_TAG"]E: " "%s(%s:%d):" fmt "\n", __time_str__, _FUNC_NAME_, _FILE_NAME_, __LINE__, ##arg);    \
                LOG_EXIT(-1);   \
            }
#else
    #define _HEXOUT(fmt, buf, length)   \
            {               \
                int _i_ = 0;      \
                printf("["LOGGER_TAG"]HEX: " "%s(%s:%d):length=%u, HEXOUTPUT:\n" fmt, _FUNC_NAME_, _FILE_NAME_, __LINE__, length);   \
                for(_i_=0; _i_ < (length); _i_++)       \
                {           \
                    printf(" %02X", (buf)[_i_]);   \
                }           \
                printf( "\n");   \
            }
    #define _DBG(fmt, arg...)   \
            printf("["LOGGER_TAG"]D: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##arg)
    #define _INFO(fmt, arg...)  \
            printf("["LOGGER_TAG"]I: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##arg)
    #define _WARN(fmt, arg...)  \
            printf("["LOGGER_TAG"]W: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##arg)
    #define _ERR(fmt, arg...)   \
            {   \
                printf("["LOGGER_TAG"]E: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##arg);    \
                LOG_EXIT(-1);   \
            }
#endif



// API of Logger.
#if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    #define LOGHEX(fmt, buf, length)       _HEXOUT(LOG_COLOR_BLUE fmt LOG_COLOR_END, buf, length)
    #define LOGD(fmt, arg...)              _DBG(LOG_COLOR_BLUE fmt LOG_COLOR_END, ##arg)
    #define LOGW(fmt, arg...)              _WARN(LOG_COLOR_BROWN fmt LOG_COLOR_END, ##arg)
    #define LOGERR(fmt, arg...)            _ERR(LOG_COLOR_RED fmt LOG_COLOR_END, ##arg)
    #define LOGI(fmt, arg...)              _INFO(fmt, ##arg)
#elif (LOG_LEVEL == LOG_LEVEL_INFO)
    #define LOGHEX(fmt, buf, length)
    #define LOGD(fmt, arg...)
    #define LOGW(fmt, arg...)              _WARN(LOG_COLOR_BROWN fmt LOG_COLOR_END, ##arg)
    #define LOGERR(fmt, arg...)            _ERR(LOG_COLOR_RED fmt LOG_COLOR_END, ##arg)
    #define LOGI(fmt, arg...)              _INFO(fmt, ##arg)
#elif (LOG_LEVEL == LOG_LEVEL_WARN)
    #define LOGHEX(fmt, buf, length)
    #define LOGD(fmt, arg...)
    #define LOGW(fmt, arg...)              _WARN(LOG_COLOR_BROWN fmt LOG_COLOR_END, ##arg)
    #define LOGERR(fmt, arg...)            _ERR(LOG_COLOR_RED fmt LOG_COLOR_END, ##arg)
    #define LOGI(fmt, arg...)
#elif (LOG_LEVEL == LOG_LEVEL_ERR)
    #define LOGHEX(fmt, buf, length)
    #define LOGD(fmt, arg...)
    #define LOGW(fmt, arg...)
    #define LOGERR(fmt, arg...)            _ERR(LOG_COLOR_RED fmt LOG_COLOR_END, ##arg)
    #define LOGI(fmt, arg...)
#endif


#endif /* LOGGER_KES_H_ */
