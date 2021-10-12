#include <unistd.h>     // open , close
#include <termios.h>    // B115200, CS8 등 상수 정의
#include <fcntl.h>      // O_RDWR , O_NOCTTY 등의 상수 정의
#include <sys/ioctl.h>
#include <cstring>      // memset
#include <stdexcept>

#include <uart.h>
#include <logger.h>

#ifndef TIOCMODG
#   ifdef TIOCMGET
#       define TIOCMODG TIOCMGET
#   else
#       ifdef MCGETA
#           define TIOCMODG MCGETA
#       endif
#   endif
#endif

#ifndef TIOCMODS
#   ifdef TIOCMSET
#       define TIOCMODS TIOCMSET
#   else
#       ifdef MCSETA
#           define TIOCMODS MCSETA
#       endif
#   endif
#endif

namespace uart {

constexpr const uint32_t IUart::READ_BUF_SIZE;

/********************************
 * Public Function Definition.
 */
IUart::IUart(std::atomic<bool>& continue_var)
: _m_is_continue_(continue_var) {
    clear();
}

IUart::~IUart(void) {
    if( _m_fd_ > 0 ) {
        close(_m_fd_);  // close device-file.
        _m_fd_ = 0;
    }
    clear();
}


/********************************
 * Protected Function Definition.
 */
void IUart::init( const char* UART_PATH, Tbr baud_rate ) {
    try {
        // Open device-file
        _m_fd_ = open( UART_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK );
        if ( _m_fd_ < 0 ) {
            std::string err = "Can not open UART_PATH(" + std::string(UART_PATH) + ")";
            throw std::runtime_error(err);
        }

        // Allocate memory-buffer to read data from Uart-Device.
        _m_read_buf_.reserve( READ_BUF_SIZE );

        // Set environment for Serial-Port Communication.
        set_uart( _m_fd_, baud_rate, 8, 'N', 1, false, false );

        // Set Event-flag for Poll-processing.
        _m_poll_events_.fd        = _m_fd_;
        _m_poll_events_.events    = POLLIN;
        _m_poll_events_.revents   = 0;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

void IUart::read_data( int fd, std::vector<uint8_t>& buffer ) {
    LOGI("Get GPS-data.");
    
    try {
        ssize_t msg_size = 0;
        size_t read_buf_size = _m_read_buf_.capacity();

        if( fd <= 0 ) {
            throw std::logic_error("File-descriptor is invalid-value.");
        }

        if( read_buf_size <= 0 ) {
            throw std::logic_error("read_buffer is under allocated rather than 10-byte. Please check it.");
        }
        msg_size = read_buf_size;
        buffer.clear();

        while( msg_size == read_buf_size ) {
            // return value description
            //   0 : End of Field.
            //  -1 : Error. (session is Closed by peer)
            // > 0 : The number of received message.
            msg_size = read_line( fd, (void*)(_m_read_buf_.data()), read_buf_size ); // Blocking Function.
            if( 0 > msg_size || msg_size > read_buf_size ) {
                std::string err = "msg_size(" + std::to_string(msg_size) + ") have to be as '0 <= msg_size && msg_size <= read_buf_size'.";
                throw std::range_error(err);
            }

            if( msg_size > 0 ){
                buffer.insert( buffer.end(), _m_read_buf_.data(), _m_read_buf_.data() + msg_size );
                // LOGHEX("msg=", buffer.data(), buffer.size());
            }
        }

        // if( buffer.size() > 0 ) {
        //     LOGHEX("msg=", buffer.data(), buffer.size());
        // }
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}


/********************************
 * Private Function Definition.
 */
void IUart::clear(void) {
    _m_fd_ = 0;
    std::memset(&_m_poll_events_, 0, sizeof(_m_poll_events_));
    _m_read_buf_.clear();
}

/***
 * fd           : file descriptor               [ > 0  ]
 * baud_rate    : speed of Uart communication   [ E_BR_9600, E_BR_115200 ]
 * data_bit     : the number of bits for data   [ 7, 8 ]
 * parity       : mode for parity-check         [ 'N': No parity-check, 'E': Even parity-check 'O': Odd parity-check ]
 * stop_bit     : the number of bits for stop   [ 1 ]
 * hw_flow      : H/W flow control flag.        [ false, true ]
 * sw_flow      : S/W flow control flag.        [ false, true ]
 */
void IUart::set_uart( int fd, Tbr baud_rate, uint8_t data_bit, char parity, uint8_t stop_bit, bool hw_flow, bool sw_flow ) {
    int mcs;
    struct termios    newtio;
    std::memset(&newtio, 0, sizeof(newtio) );

    try {
        // set Baud-Rate of Uart
        switch( baud_rate ){
        case Tbr::E_BR_9600:
            newtio.c_cflag       = B9600;
            break;
        case Tbr::E_BR_115200:
            newtio.c_cflag       = B115200;
            break;
        default:
            throw std::invalid_argument("Not Supported Uart baud-rate Case.");
        }

        newtio.c_cflag       |= (CS8 | CLOCAL | CREAD);
        newtio.c_iflag       = IGNBRK;
        newtio.c_oflag       = ONLCR | OPOST;
        newtio.c_lflag       = 0;
        newtio.c_cc[VTIME]   = 10;
        newtio.c_cc[VMIN]    = 1;

        // set S/W flow contorl
        newtio.c_iflag = sw_flow ? newtio.c_iflag | (IXON|IXOFF) : newtio.c_iflag & ~(IXON|IXOFF|IXANY);

        // set H/W flow contorl
        ioctl( fd, TIOCMODG, &mcs );
        mcs |= TIOCM_RTS;
        ioctl( fd, TIOCMODS, &mcs );
        newtio.c_cflag = hw_flow ? newtio.c_cflag | CRTSCTS : newtio.c_cflag & ~CRTSCTS;

        // set parity
        switch( parity ) {
        case 'N':   // No parity-check
            newtio.c_cflag &= ~(PARENB | PARODD);
            break;
        case 'E':   // Even parity-check
            newtio.c_cflag &= ~PARODD;
            newtio.c_cflag |= PARENB;
            break;
        case 'O':   // Odd parity-check
            newtio.c_cflag |= (PARENB | PARODD);
            break;
        default:
            throw std::invalid_argument("Not Supported Uart parity Case.");
        }

        tcflush(fd, TCIFLUSH);
        tcsetattr(fd, TCSANOW, &newtio);
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
}

IUart::TState IUart::wait_data( struct pollfd& poll_events ) {
    int poll_state = 0;
    int timeout_msec = 1000;
    TState state = TState::E_STATE_ERROR;

    try {
        while( _m_is_continue_ == true ) {
            poll_state = poll( (struct pollfd*)&poll_events, 1, timeout_msec ); // Block API
            if( poll_state == 0 ) {
                LOGD("Occure Time-Out of poll-processing.");
                continue;
            }

            if( poll_state < 0 ) {
                throw std::runtime_error("Occure Critical-Error of poll-processing.");
            }
            else if( poll_events.revents & POLLERR ) {
                throw std::runtime_error("Received POLL-ERROR flag during poll-processing.");
            }
            else if( poll_events.revents & POLLNVAL ) {
                throw std::runtime_error("Received Invalid-POLL-REQ flag during poll-processing.");
            }

            /** Get Result */
            if( poll_events.revents & POLLHUP ) {
                LOGERR("Received Disconnected with FD during poll-processing.");
                state = TState::E_STATE_DISCONNECTED;
            }
            else if ( poll_events.revents & POLLIN ) {
                state = TState::E_STATE_GET_DATA;
            }
            break;
        }
    }
    catch ( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }

    return state;
}

int IUart::read_line( int fd, void* buffer, size_t buffer_size ) {
    ssize_t msg_size = 0;
    size_t idx = 0;

    try {
        while( idx < buffer_size ) {
            if( wait_data(_m_poll_events_) != TState::E_STATE_GET_DATA ) {  // Block API
                throw std::out_of_range("Disconnected with Uart-Module.");
            }

            msg_size = 0;
            do {
                idx += msg_size;
                msg_size = read( fd, buffer + idx, 1 ); // Non-Blocking Function. (Read 1 byte per 1-OP.)
                if( msg_size > 0 && *(char*)(buffer + idx + msg_size -1) == '\n' ) {
                    return idx + msg_size;  // LF == '\n' == 0x0A
                }
            } while( msg_size > 0 && idx < buffer_size );
        }
    }
    catch( const std::out_of_range& e ) {
        LOGW("%s", e.what());
        idx = -1;
    }
    catch( const std::exception& e ) {
        LOGERR("%s", e.what());
        throw e;
    }
    return idx;
}



}   // uart