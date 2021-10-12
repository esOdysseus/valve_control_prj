#ifndef _H_CLASS_UART_CLASS_KES_
#define _H_CLASS_UART_CLASS_KES_

#include <vector>
#include <atomic>
#include <cstdint>

#include <sys/poll.h>

namespace uart {


class IUart {
protected:
    using Tbr = enum class _enum_baud_rate_ {
        E_BR_NONE = 0,
        E_BR_9600,
        E_BR_115200
    };

    using TState = enum class _enum_state_ {
        /* Global State: It can set _m_state_ variable */
        E_STATE_INACTIVE = 0,
        E_STATE_ACTIVE,
        /* Local State: Use as return value */
        E_STATE_GET_DATA = 2,
        E_STATE_DISCONNECTED,
        E_STATE_BLOCK_WRITE,
        E_STATE_ERROR
    };

public:
    IUart(std::atomic<bool>& continue_var);

    ~IUart(void);

protected:
    void init( const char* UART_PATH, Tbr baud_rate );

    void read_data( int fd, std::vector<uint8_t>& buffer );

private:
    IUart(void) = delete;

    void clear(void);

    void set_uart( int fd, Tbr baud_rate, uint8_t data_bit, char parity, uint8_t stop_bit, bool hw_flow, bool sw_flow );

    TState wait_data( struct pollfd& poll_events );

    int read_line( int fd, void* buffer, size_t buffer_size );

protected:
    /** Uart Poll Events */
    int _m_fd_;

    static constexpr const uint32_t READ_BUF_SIZE = 1024U;

private:
    struct pollfd _m_poll_events_;

    std::vector<uint8_t> _m_read_buf_;

    std::atomic<bool>& _m_is_continue_;

};


}   // uart

#endif // _H_CLASS_UART_CLASS_KES_