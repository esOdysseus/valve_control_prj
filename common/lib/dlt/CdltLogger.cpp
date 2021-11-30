#include <cstdio>
#include <cstring>
#include <stdarg.h>

#include <log_level.h>
#include <time_kes.h>

#include <CdltLogger.h>


namespace dlt {

constexpr const uint32_t CdltLogger::MAX_EXTRA_SIZE;

/******************************
 * Public function definition
 */
CdltLogger& CdltLogger::get_instance(const char* appid, const char* cid) {
    static CdltLogger instance(appid, cid);
    return instance;
}

CdltLogger::~CdltLogger(void) {
    DLT_UNREGISTER_CONTEXT(_m_ctx_);
    DLT_UNREGISTER_APP_FLUSH_BUFFERED_LOGS();
}

void CdltLogger::print(const uint8_t* buf, size_t length, uint32_t level, const char* fmt, ...) {
    std::vector<char> str;
    int written_length = 0;
    size_t max_length = std::strlen(fmt) + (3*length) + MAX_EXTRA_SIZE;
    if( max_length <= MAX_EXTRA_SIZE ) {
        return ;
    }

    auto itr = _m_cvt_level_.find(level);
    if( itr == _m_cvt_level_.end() ) {
        return ;
    }

    try {
        va_list args;
        va_start(args, fmt);

        // make Completion String
        str.reserve( max_length + 1 );
        str.assign( max_length, NULL );
        written_length = std::vsnprintf( str.data(), max_length + 1, fmt, args );
        if( written_length <= 0 ) {
            printf("[%s]%s:%u:FATAL DLT-Log writing is failed.\n", _FILE_NAME_, _FUNC_NAME_, __LINE__);
            return ;
        }

        va_end(args);

        // append Hex strings
        if( buf != NULL ) {
            int result = 0;
            for(auto i=0; i < length && written_length < max_length; i++ ) {
                result = append_hex(str.data() + written_length, max_length-written_length+1, buf[i]);
                if( result <= 0 ) {
                    break;
                }
                written_length += result;
            }
        }

        // printf("result-str=%s", str.data());
        print_raw( itr->second, str.data() );
    }
    catch( const std::exception& e ) {
        printf("ERROR:(print_hex) %s\n", e.what());
    }
}


/******************************
 * Private function definition
 */
CdltLogger::CdltLogger(const char* appid, const char* cid) {
    std::string app_desp;
    std::string cid_desp;

    try {
        _m_cvt_level_[LOG_LEVEL_ERR] = Tlevel::ERROR;
        _m_cvt_level_[LOG_LEVEL_WARN] = Tlevel::WARN;
        _m_cvt_level_[LOG_LEVEL_INFO] = Tlevel::INFO;
        _m_cvt_level_[LOG_LEVEL_DEBUG] = Tlevel::DEBUG;

        // Set DLT.
        app_desp = std::string(appid) + " Application is Started.";
        cid_desp = std::string(cid) + " Context is set.";

        printf(">> Register APPID(%s), CID(%s)\n", appid, cid);
        DLT_REGISTER_APP(appid, app_desp.data());
        DLT_REGISTER_CONTEXT_LL_TS(_m_ctx_, cid, cid_desp.data(), static_cast<uint32_t>(Tlevel::DEBUG), DLT_TRACE_STATUS_OFF);
    }
    catch( const std::exception& e ) {
        printf("ERROR:(CdltLogger) %s\n", e.what());
        throw ;
    }
}

void CdltLogger::print_raw( Tlevel level, const char* str ) {
    uint32_t timestamp = (uint32_t)(::time_pkg::CTime::get<double>());
    DLT_LOG_TS(_m_ctx_, static_cast<DltLogLevelType>(level), timestamp, DLT_STRING(str));
}

int CdltLogger::append_hex(char* str, size_t str_capa, uint8_t value) {
    if( str_capa < 4 ) {    // If string capability is under th 4 bytes, then infom exit-signal.
        return -1;
    }

    str[0] = ' ';
    str[1] = (char)((value/16) + (uint8_t)('0'));
    str[2] = (char)((value%16) + (uint8_t)('0'));
    str[3] = NULL;
    return 3;
}


} // dlt