/*
 * shared_mutex_kes.h
 * 
 * Description : alternative-standard library of shared_mutex for c++11 supporting.
 *
 *  Created on: Feb 12, 2020
 *  Author: eunseok, kim
 */

#ifndef LIB_SHARED_MUTEX_KES_H_
#define LIB_SHARED_MUTEX_KES_H_


#pragma GCC system_header

#if __cplusplus >= 201103L

#include <condition_variable>

namespace std
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  /**
   * @addtogroup mutexes
   * @{
   */

#ifdef _GLIBCXX_HAS_GTHREADS

  /// @cond undocumented

#if _GLIBCXX_USE_PTHREAD_RWLOCK_T
#ifdef __gthrw
#define _GLIBCXX_GTHRW(name) \
  __gthrw(pthread_ ## name); \
  static inline int \
  __glibcxx_ ## name (pthread_rwlock_t *__rwlock) \
  { \
    if (__gthread_active_p ()) \
      return __gthrw_(pthread_ ## name) (__rwlock); \
    else \
      return 0; \
  }
  _GLIBCXX_GTHRW(rwlock_rdlock)
  _GLIBCXX_GTHRW(rwlock_tryrdlock)
  _GLIBCXX_GTHRW(rwlock_wrlock)
  _GLIBCXX_GTHRW(rwlock_trywrlock)
  _GLIBCXX_GTHRW(rwlock_unlock)
# ifndef PTHREAD_RWLOCK_INITIALIZER
  _GLIBCXX_GTHRW(rwlock_destroy)
  __gthrw(pthread_rwlock_init);
  static inline int
  __glibcxx_rwlock_init (pthread_rwlock_t *__rwlock)
  {
    if (__gthread_active_p ())
      return __gthrw_(pthread_rwlock_init) (__rwlock, NULL);
    else
      return 0;
  }
# endif
# if _GTHREAD_USE_MUTEX_TIMEDLOCK
   __gthrw(pthread_rwlock_timedrdlock);
  static inline int
  __glibcxx_rwlock_timedrdlock (pthread_rwlock_t *__rwlock,
				const timespec *__ts)
  {
    if (__gthread_active_p ())
      return __gthrw_(pthread_rwlock_timedrdlock) (__rwlock, __ts);
    else
      return 0;
  }
   __gthrw(pthread_rwlock_timedwrlock);
  static inline int
  __glibcxx_rwlock_timedwrlock (pthread_rwlock_t *__rwlock,
				const timespec *__ts)
  {
    if (__gthread_active_p ())
      return __gthrw_(pthread_rwlock_timedwrlock) (__rwlock, __ts);
    else
      return 0;
  }
# endif
#else
  static inline int
  __glibcxx_rwlock_rdlock (pthread_rwlock_t *__rwlock)
  { return pthread_rwlock_rdlock (__rwlock); }
  static inline int
  __glibcxx_rwlock_tryrdlock (pthread_rwlock_t *__rwlock)
  { return pthread_rwlock_tryrdlock (__rwlock); }
  static inline int
  __glibcxx_rwlock_wrlock (pthread_rwlock_t *__rwlock)
  { return pthread_rwlock_wrlock (__rwlock); }
  static inline int
  __glibcxx_rwlock_trywrlock (pthread_rwlock_t *__rwlock)
  { return pthread_rwlock_trywrlock (__rwlock); }
  static inline int
  __glibcxx_rwlock_unlock (pthread_rwlock_t *__rwlock)
  { return pthread_rwlock_unlock (__rwlock); }
  static inline int
  __glibcxx_rwlock_destroy(pthread_rwlock_t *__rwlock)
  { return pthread_rwlock_destroy (__rwlock); }
  static inline int
  __glibcxx_rwlock_init(pthread_rwlock_t *__rwlock)
  { return pthread_rwlock_init (__rwlock, NULL); }
# if _GTHREAD_USE_MUTEX_TIMEDLOCK
  static inline int
  __glibcxx_rwlock_timedrdlock (pthread_rwlock_t *__rwlock,
				const timespec *__ts)
  { return pthread_rwlock_timedrdlock (__rwlock, __ts); }
  static inline int
  __glibcxx_rwlock_timedwrlock (pthread_rwlock_t *__rwlock,
				const timespec *__ts)
  { return pthread_rwlock_timedwrlock (__rwlock, __ts); }
# endif
#endif

  /// A shared mutex type implemented using pthread_rwlock_t.
  class __shared_mutex_pthread
  {
//    friend class shared_timed_mutex;

#ifdef PTHREAD_RWLOCK_INITIALIZER
    pthread_rwlock_t	_M_rwlock = PTHREAD_RWLOCK_INITIALIZER;

  public:
    __shared_mutex_pthread() = default;
    ~__shared_mutex_pthread() = default;
#else
    pthread_rwlock_t	_M_rwlock;

  public:
    __shared_mutex_pthread()
    {
      int __ret = __glibcxx_rwlock_init(&_M_rwlock, NULL);
      if (__ret == ENOMEM)
	__throw_bad_alloc();
      else if (__ret == EAGAIN)
	__throw_system_error(int(errc::resource_unavailable_try_again));
      else if (__ret == EPERM)
	__throw_system_error(int(errc::operation_not_permitted));
      // Errors not handled: EBUSY, EINVAL
      __glibcxx_assert(__ret == 0);
    }

    ~__shared_mutex_pthread()
    {
      int __ret __attribute((__unused__)) = __glibcxx_rwlock_destroy(&_M_rwlock);
      // Errors not handled: EBUSY, EINVAL
      __glibcxx_assert(__ret == 0);
    }
#endif

    __shared_mutex_pthread(const __shared_mutex_pthread&) = delete;
    __shared_mutex_pthread& operator=(const __shared_mutex_pthread&) = delete;

    void
    lock()
    {
      int __ret = __glibcxx_rwlock_wrlock(&_M_rwlock);
      if (__ret == EDEADLK)
	__throw_system_error(int(errc::resource_deadlock_would_occur));
      // Errors not handled: EINVAL
      __glibcxx_assert(__ret == 0);
    }

    bool
    try_lock()
    {
      int __ret = __glibcxx_rwlock_trywrlock(&_M_rwlock);
      if (__ret == EBUSY) return false;
      // Errors not handled: EINVAL
      __glibcxx_assert(__ret == 0);
      return true;
    }

    void
    unlock()
    {
      int __ret __attribute((__unused__)) = __glibcxx_rwlock_unlock(&_M_rwlock);
      // Errors not handled: EPERM, EBUSY, EINVAL
      __glibcxx_assert(__ret == 0);
    }

    // Shared ownership

    void
    lock_shared()
    {
      int __ret;
      // We retry if we exceeded the maximum number of read locks supported by
      // the POSIX implementation; this can result in busy-waiting, but this
      // is okay based on the current specification of forward progress
      // guarantees by the standard.
      do
	__ret = __glibcxx_rwlock_rdlock(&_M_rwlock);
      while (__ret == EAGAIN);
      if (__ret == EDEADLK)
	__throw_system_error(int(errc::resource_deadlock_would_occur));
      // Errors not handled: EINVAL
      __glibcxx_assert(__ret == 0);
    }

    bool
    try_lock_shared()
    {
      int __ret = __glibcxx_rwlock_tryrdlock(&_M_rwlock);
      // If the maximum number of read locks has been exceeded, we just fail
      // to acquire the lock.  Unlike for lock(), we are not allowed to throw
      // an exception.
      if (__ret == EBUSY || __ret == EAGAIN) return false;
      // Errors not handled: EINVAL
      __glibcxx_assert(__ret == 0);
      return true;
    }

    void
    unlock_shared()
    {
      unlock();
    }

    void* native_handle() { return &_M_rwlock; }
  };
#endif

#if ! (_GLIBCXX_USE_PTHREAD_RWLOCK_T && _GTHREAD_USE_MUTEX_TIMEDLOCK)
  /// A shared mutex type implemented using std::condition_variable.
  class __shared_mutex_cv
  {
//    friend class shared_timed_mutex;

    // Based on Howard Hinnant's reference implementation from N2406.

    // The high bit of _M_state is the write-entered flag which is set to
    // indicate a writer has taken the lock or is queuing to take the lock.
    // The remaining bits are the count of reader locks.
    //
    // To take a reader lock, block on gate1 while the write-entered flag is
    // set or the maximum number of reader locks is held, then increment the
    // reader lock count.
    // To release, decrement the count, then if the write-entered flag is set
    // and the count is zero then signal gate2 to wake a queued writer,
    // otherwise if the maximum number of reader locks was held signal gate1
    // to wake a reader.
    //
    // To take a writer lock, block on gate1 while the write-entered flag is
    // set, then set the write-entered flag to start queueing, then block on
    // gate2 while the number of reader locks is non-zero.
    // To release, unset the write-entered flag and signal gate1 to wake all
    // blocked readers and writers.
    //
    // This means that when no reader locks are held readers and writers get
    // equal priority. When one or more reader locks is held a writer gets
    // priority and no more reader locks can be taken while the writer is
    // queued.

    // Only locked when accessing _M_state or waiting on condition variables.
    mutex		_M_mut;
    // Used to block while write-entered is set or reader count at maximum.
    condition_variable	_M_gate1;
    // Used to block queued writers while reader count is non-zero.
    condition_variable	_M_gate2;
    // The write-entered flag and reader count.
    unsigned		_M_state;

    static constexpr unsigned _S_write_entered
      = 1U << (sizeof(unsigned)*__CHAR_BIT__ - 1);
    static constexpr unsigned _S_max_readers = ~_S_write_entered;

    // Test whether the write-entered flag is set. _M_mut must be locked.
    bool _M_write_entered() const { return _M_state & _S_write_entered; }

    // The number of reader locks currently held. _M_mut must be locked.
    unsigned _M_readers() const { return _M_state & _S_max_readers; }

  public:
    __shared_mutex_cv() : _M_state(0) {}

    ~__shared_mutex_cv()
    {
      __glibcxx_assert( _M_state == 0 );
    }

    __shared_mutex_cv(const __shared_mutex_cv&) = delete;
    __shared_mutex_cv& operator=(const __shared_mutex_cv&) = delete;

    // Exclusive ownership

    void
    lock()
    {
      unique_lock<mutex> __lk(_M_mut);
      // Wait until we can set the write-entered flag.
      _M_gate1.wait(__lk, [=]{ return !_M_write_entered(); });
      _M_state |= _S_write_entered;
      // Then wait until there are no more readers.
      _M_gate2.wait(__lk, [=]{ return _M_readers() == 0; });
    }

    bool
    try_lock()
    {
      unique_lock<mutex> __lk(_M_mut, try_to_lock);
      if (__lk.owns_lock() && _M_state == 0)
	{
	  _M_state = _S_write_entered;
	  return true;
	}
      return false;
    }

    void
    unlock()
    {
      lock_guard<mutex> __lk(_M_mut);
      __glibcxx_assert( _M_write_entered() );
      _M_state = 0;
      // call notify_all() while mutex is held so that another thread can't
      // lock and unlock the mutex then destroy *this before we make the call.
      _M_gate1.notify_all();
    }

    // Shared ownership

    void
    lock_shared()
    {
      unique_lock<mutex> __lk(_M_mut);
      _M_gate1.wait(__lk, [=]{ return _M_state < _S_max_readers; });
      ++_M_state;
    }

    bool
    try_lock_shared()
    {
      unique_lock<mutex> __lk(_M_mut, try_to_lock);
      if (!__lk.owns_lock())
	return false;
      if (_M_state < _S_max_readers)
	{
	  ++_M_state;
	  return true;
	}
      return false;
    }

    void
    unlock_shared()
    {
      lock_guard<mutex> __lk(_M_mut);
      __glibcxx_assert( _M_readers() > 0 );
      auto __prev = _M_state--;
      if (_M_write_entered())
	{
	  // Wake the queued writer if there are no more readers.
	  if (_M_readers() == 0)
	    _M_gate2.notify_one();
	  // No need to notify gate1 because we give priority to the queued
	  // writer, and that writer will eventually notify gate1 after it
	  // clears the write-entered flag.
	}
      else
	{
	  // Wake any thread that was blocked on reader overflow.
	  if (__prev == _S_max_readers)
	    _M_gate1.notify_one();
	}
    }
  };
#endif
  /// @endcond

#if __cplusplus >= 201103L
  /// The standard shared mutex type.
  class shared_mutex
  {
  public:
    shared_mutex() = default;
    ~shared_mutex() = default;

    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;

    // Exclusive ownership

    void lock() { _M_impl.lock(); }
    bool try_lock() { return _M_impl.try_lock(); }
    void unlock() { _M_impl.unlock(); }

    // Shared ownership

    void lock_shared() { _M_impl.lock_shared(); }
    bool try_lock_shared() { return _M_impl.try_lock_shared(); }
    void unlock_shared() { _M_impl.unlock_shared(); }

#if _GLIBCXX_USE_PTHREAD_RWLOCK_T
    typedef void* native_handle_type;
    native_handle_type native_handle() { return _M_impl.native_handle(); }

  private:
    __shared_mutex_pthread _M_impl;
#else
  private:
    __shared_mutex_cv _M_impl;
#endif
  };
#endif // C++11

#endif // _GLIBCXX_HAS_GTHREADS

  /// shared_lock
  template<typename _Mutex>
    class shared_lock
    {
    public:
      typedef _Mutex mutex_type;

      // Shared locking

      shared_lock() noexcept : _M_pm(nullptr), _M_owns(false) { }

      explicit
      shared_lock(mutex_type& __m)
      : _M_pm(std::__addressof(__m)), _M_owns(true)
      { __m.lock_shared(); }

      shared_lock(mutex_type& __m, defer_lock_t) noexcept
      : _M_pm(std::__addressof(__m)), _M_owns(false) { }

      shared_lock(mutex_type& __m, try_to_lock_t)
      : _M_pm(std::__addressof(__m)), _M_owns(__m.try_lock_shared()) { }

      shared_lock(mutex_type& __m, adopt_lock_t)
      : _M_pm(std::__addressof(__m)), _M_owns(true) { }

      ~shared_lock()
      {
		if (_M_owns)
		  _M_pm->unlock_shared();
      }

      shared_lock(shared_lock const&) = delete;
      shared_lock& operator=(shared_lock const&) = delete;

      shared_lock(shared_lock&& __sl) noexcept : shared_lock()
      { swap(__sl); }

      shared_lock&
      operator=(shared_lock&& __sl) noexcept
      {
		shared_lock(std::move(__sl)).swap(*this);
		return *this;
      }

      void
      lock()
      {
		_M_lockable();
		_M_pm->lock_shared();
		_M_owns = true;
      }

      bool
      try_lock()
      {
		_M_lockable();
		return _M_owns = _M_pm->try_lock_shared();
      }

      void
      unlock()
      {
		if (!_M_owns)
		  __throw_system_error(int(errc::resource_deadlock_would_occur));
		_M_pm->unlock_shared();
		_M_owns = false;
      }

      // Setters

      void
      swap(shared_lock& __u) noexcept
      {
		std::swap(_M_pm, __u._M_pm);
		std::swap(_M_owns, __u._M_owns);
      }

      // Getters
      bool owns_lock() const noexcept { return _M_owns; }

      explicit operator bool() const noexcept { return _M_owns; }

      mutex_type* mutex() const noexcept { return _M_pm; }

    private:
      void
      _M_lockable() const
      {
		if (_M_pm == nullptr)
		  __throw_system_error(int(errc::operation_not_permitted));
		if (_M_owns)
		  __throw_system_error(int(errc::resource_deadlock_would_occur));
      }

      mutex_type*	_M_pm;
      bool		_M_owns;
    };

  /// Swap specialization for shared_lock
  /// @relates shared_mutex
  template<typename _Mutex>
    void
    swap(shared_lock<_Mutex>& __x, shared_lock<_Mutex>& __y) noexcept
    { __x.swap(__y); }

  // @} group mutexes
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace

#endif // C++11


#endif /* LIB_SHARED_MUTEX_KES_H_ */
