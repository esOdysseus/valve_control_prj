#ifndef _H_CLASS_DLT_LOGGER_
#define _H_CLASS_DLT_LOGGER_

#include <cstdint>
#include <cstddef>
#include <map>
#include <dlt.h>

namespace dlt {


class CdltLogger {
private:
    using Tlevel = enum class E_LEVEL: uint32_t {
        ERROR = DltLogLevelType::DLT_LOG_ERROR,
        WARN = DltLogLevelType::DLT_LOG_WARN,
        INFO = DltLogLevelType::DLT_LOG_INFO,
        DEBUG = DltLogLevelType::DLT_LOG_DEBUG
    };

public:
    static CdltLogger& get_instance(const char* appid, const char* cid);

    ~CdltLogger(void);

    void print(const uint8_t* buf, size_t length, uint32_t level, const char* fmt, ...);

private:
    CdltLogger(void) = delete;
    
    CdltLogger(const char* appid, const char* cid);

    void print_raw(Tlevel level, const char* str);

    int append_hex(char* str, size_t str_capa, uint8_t value);

private:
    DLT_DECLARE_CONTEXT(_m_ctx_);

    std::map<uint32_t, Tlevel> _m_cvt_level_;

    static constexpr const uint32_t MAX_EXTRA_SIZE = 512U;

};


} // dlt


#endif // _H_CLASS_DLT_LOGGER_