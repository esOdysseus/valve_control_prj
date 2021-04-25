#ifndef _INTERFACE_MONITOR_DATA_H_
#define _INTERFACE_MONITOR_DATA_H_

#include <cassert>
#include <memory>
#include <functional>

#include <CException.h>

namespace monitor_pkg {


/********************************
 * Attention
 *   1. _ID_TYPE_        : available type-list -> default data-type like (int, string, ...)
 *   2. _FLAG_TYPE_      : available type-list -> uint8_t, uint16_t, uint32_t, uint64_t
 *   3. _ERR_TYPE_       : available type-list -> uint8_t, uint16_t, uint32_t, uint64_t
 *   4. _ENUM_MNT_FLAG_  : have to be element(EMNT_FLAG_NONE)
 *   5. _ENUM_MNT_ERR_   : have to be element(EMNT_ERROR_NONE)
 */
template <typename _ID_TYPE_,
          typename _FLAG_TYPE_, 
          typename _ERR_TYPE_, 
          typename _ENUM_MNT_FLAG_,
          typename _ENUM_MNT_ERR_>
class IMD {
public:
    using IDType = _ID_TYPE_;
    using FlagType = _FLAG_TYPE_;
    using ErrType = _ERR_TYPE_;
    using E_MNT_FLAG = _ENUM_MNT_FLAG_;
    using E_MNT_ERROR = _ENUM_MNT_ERR_;
    // using FlagHookType = bool (*)(E_MNT_FLAG pos, FlagType value);
    using FlagHookType = std::function<bool(E_MNT_FLAG pos, FlagType value)>;

public:
    IMD(IDType id, FlagHookType f_hook=NULL) {
        if ( std::is_same<FlagType, uint8_t>::value == false &&
             std::is_same<FlagType, uint16_t>::value == false && 
             std::is_same<FlagType, uint32_t>::value == false &&
             std::is_same<FlagType, uint64_t>::value == false ) {
            LOGERR("Not supported FlagType.");
            throw CException(E_ERROR::E_ERR_NOT_SUPPORTED_TYPE);
        }

        if ( std::is_same<ErrType, uint8_t>::value == false &&
             std::is_same<ErrType, uint16_t>::value == false && 
             std::is_same<ErrType, uint32_t>::value == false &&
             std::is_same<ErrType, uint64_t>::value == false ) {
            LOGERR("Not supported ErrType.");
            throw CException(E_ERROR::E_ERR_NOT_SUPPORTED_TYPE);
        }

        clear();
        init(id, f_hook);
    }

    ~IMD(void) {
        clear();
    }

    bool set_flag(E_MNT_FLAG pos, FlagType value) {
        bool res = false;
        raw_set_flag(pos, value);

        if ( _f_hook_ != NULL ) {
            res = _f_hook_(pos, value);
        }
        return res;
    }

    void set_err(E_MNT_ERROR pos, ErrType value) {
        int shift_cnt = 0;

        if ( pos == E_MNT_ERROR::EMNT_ERROR_NONE ) {
            _err_ = value;
        }
        else {
            // Assumption : pos is continuous-bitmask.
            while( ((1<<shift_cnt) & (ErrType)pos) == 0 ) {
                shift_cnt++;
                assert( shift_cnt < (sizeof(ErrType)*8) );
            }

            _err_ = (_err_ & (~(ErrType)pos)) | ((value << shift_cnt) & (ErrType)pos);
        }
    }

    FlagType get_flag(E_MNT_FLAG pos) {
        assert( pos != E_MNT_FLAG::EMNT_FLAG_NONE);
        return _flag_ & (FlagType)pos;
    }

    ErrType get_err(E_MNT_ERROR pos) {
        assert( pos != E_MNT_ERROR::EMNT_ERROR_NONE);
        return _err_ & (ErrType)pos;
    }

    IDType get_id(void) {
        return _id_;
    }

    double get_time(void) {
        return _last_send_try_time_;
    }

    double get_elapsed_time(void) {
        struct timespec t_now;
        double d_now = 0.0;

        // get current-time by REAL-TIME-CLOCK.
        assert( clock_gettime(CLOCK_REALTIME, &t_now) != -1 );  
        assert( (d_now = extract_utc2double(t_now)) > 0.0 );
        assert( (d_now-_last_send_try_time_) >= 0.0 );

        return (d_now - _last_send_try_time_);
    }

protected:
    IMD(void) = delete;

    void init(IDType id, FlagHookType f_hook=NULL) {
        _id_ = id;
        _f_hook_ = f_hook;
    }

    void raw_set_flag(E_MNT_FLAG pos, FlagType value) {
        int shift_cnt = 0;

        if ( pos == E_MNT_FLAG::EMNT_FLAG_NONE ) {
            _flag_ = value;
        }
        else {
            // Assumption : pos is continuous-bitmask.
            while( ((1<<shift_cnt) & (FlagType)pos) == 0 ) {
                shift_cnt++;
                assert( shift_cnt < (sizeof(FlagType)*8) );
            }

            _flag_ = (_flag_ & (~(FlagType)pos)) | ((value << shift_cnt) & (FlagType)pos);
        }
    }

    void set_time(void) {
        struct timespec t_now;
        double d_now = 0.0;

        try {
            // get current-time by REAL-TIME-CLOCK.
            assert( clock_gettime(CLOCK_REALTIME, &t_now) != -1 );  
            assert( (d_now = extract_utc2double(t_now)) > 0.0 );
            _last_send_try_time_ = d_now;
        }
        catch (const std::exception &e) {
            throw e;
        }
    }

    void clear(void) {
        // Need clear processing in personal class.
        _flag_ = (FlagType)E_MNT_FLAG::EMNT_FLAG_NONE;
        _err_ = (ErrType)E_MNT_ERROR::EMNT_ERROR_NONE;
        _last_send_try_time_ = 0.0;
        _f_hook_ = NULL;
    }

    double extract_utc2double(struct timespec &time) {
        double res = 0.0;

        res += (double)time.tv_sec;
        res += (double)time.tv_nsec / 1000000000.0;
        // LOGD( "extracted UTC time = %f", res );
        return res;
    }

protected:
    IDType _id_;

    double _last_send_try_time_;

private:
    FlagType _flag_;    // retrans-cnt , sent , ack-rcv , act-ok , err-occure

    ErrType _err_;

    FlagHookType _f_hook_;

};


}

#endif // _INTERFACE_MONITOR_DATA_H_