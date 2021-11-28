#ifndef _H_LOCAL_PRINT_AS_STDOUT_
#define _H_LOCAL_PRINT_AS_STDOUT_

#include <stdio.h>

#ifdef LOG_TIME_ENABLE
    #include <assert.h>
    #include <time.h>
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
    #define _HEXOUT(buf, length, fmt, ...)   \
            {               \
                int _i_ = 0;      \
                printf("["LOGGER_TAG"]HEX: " "%s(%s:%d):length=%u, HEXOUTPUT:\n" fmt, _FUNC_NAME_, _FILE_NAME_, __LINE__, length, ##__VA_ARGS__);   \
                for(_i_=0; _i_ < (length); _i_++)       \
                {           \
                    printf(" %02X", (buf)[_i_]);   \
                }           \
                printf( "\n");   \
            }
    #define _DBG(fmt, ...)   \
            {   \
                char __time_str__[TIME_BUF_SIZE]="";  \
                GET_CUR_TIME(__time_str__, TIME_BUF_SIZE); \
                printf("%s""["LOGGER_TAG"]D: " "%s(%s:%d):" fmt "\n", __time_str__, _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__); \
            }
    #define _INFO(fmt, ...)  \
            {   \
                char __time_str__[TIME_BUF_SIZE]="";  \
                GET_CUR_TIME(__time_str__, TIME_BUF_SIZE); \
                printf("%s""["LOGGER_TAG"]I: " "%s(%s:%d):" fmt "\n", __time_str__, _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__); \
            }
    #define _WARN(fmt, ...)  \
            {   \
                char __time_str__[TIME_BUF_SIZE]="";  \
                GET_CUR_TIME(__time_str__, TIME_BUF_SIZE); \
                printf("%s""["LOGGER_TAG"]W: " "%s(%s:%d):" fmt "\n", __time_str__, _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__); \
            }
    #define _ERR(fmt, ...)   \
            {   \
                char __time_str__[TIME_BUF_SIZE]="";  \
                GET_CUR_TIME(__time_str__, TIME_BUF_SIZE); \
                printf("%s""["LOGGER_TAG"]E: " "%s(%s:%d):" fmt "\n", __time_str__, _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__);    \
                LOG_EXIT(-1);   \
            }
#else
    #define _HEXOUT(buf, length, fmt, ...)   \
            {               \
                int _i_ = 0;      \
                printf("["LOGGER_TAG"]HEX: " "%s(%s:%d):length=%u, HEXOUTPUT:\n" fmt, _FUNC_NAME_, _FILE_NAME_, __LINE__, length, ##__VA_ARGS__);   \
                for(_i_=0; _i_ < (length); _i_++)       \
                {           \
                    printf(" %02X", (buf)[_i_]);   \
                }           \
                printf( "\n");   \
            }
    #define _DBG(fmt, ...)   \
            printf("["LOGGER_TAG"]D: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__)
    #define _INFO(fmt, ...)  \
            printf("["LOGGER_TAG"]I: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__)
    #define _WARN(fmt, ...)  \
            printf("["LOGGER_TAG"]W: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__)
    #define _ERR(fmt, ...)   \
            {   \
                printf("["LOGGER_TAG"]E: " "%s(%s:%d):" fmt "\n", _FUNC_NAME_, _FILE_NAME_, __LINE__, ##__VA_ARGS__);    \
                LOG_EXIT(-1);   \
            }
#endif



// API of Logger.
#define _PRINT_HEX_(buf, length, fmt, ...)       _HEXOUT(buf, length, LOG_COLOR_BLUE fmt LOG_COLOR_END, ##__VA_ARGS__)
#define _PRINT_D_(fmt, ...)              _DBG(LOG_COLOR_BLUE fmt LOG_COLOR_END, ##__VA_ARGS__)
#define _PRINT_W_(fmt, ...)              _WARN(LOG_COLOR_BROWN fmt LOG_COLOR_END, ##__VA_ARGS__)
#define _PRINT_ERR_(fmt, ...)            _ERR(LOG_COLOR_RED fmt LOG_COLOR_END, ##__VA_ARGS__)
#define _PRINT_I_(fmt, ...)              _INFO(fmt, ##__VA_ARGS__)





#endif // _H_LOCAL_PRINT_AS_STDOUT_