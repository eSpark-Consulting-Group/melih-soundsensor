#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <chrono>
#include <functional>
#include <memory>
#include <bits/functexcept.h>
#include <bits/functional_hash.h>

namespace core {

template <typename IdType>
struct hash;

class Thread {
public:
    using native_handle_type = TaskHandle_t;

    struct _Impl_base;
    using __shared_base_type = std::shared_ptr<_Impl_base>;

    class ThreadId {
        native_handle_type m_threadHandle = nullptr;

    public:
        ThreadId() noexcept
        : m_threadHandle() {}

        explicit ThreadId( native_handle_type __id )
        : m_threadHandle( __id ) {}

    private:
        friend class Thread;
        friend class hash<Thread::ThreadId>;

        friend bool operator==( const Thread::ThreadId& __x, const Thread::ThreadId& __y ) noexcept {
            return __x.m_threadHandle == __y.m_threadHandle;
        }

        friend bool operator<( const Thread::ThreadId& __x, const Thread::ThreadId& __y ) noexcept {
            return __x.m_threadHandle < __y.m_threadHandle;
        }

        template <class _CharT, class _Traits>
        friend std::basic_ostream<_CharT, _Traits>& operator<<( std::basic_ostream<_CharT, _Traits>& __out,
                                                                Thread::ThreadId __id );
    };  // class ThreadId

    // Simple base type that the templatized, derived class containing
    // an arbitrary functor can be converted to and called.
    struct _Impl_base {
        __shared_base_type _M_this_ptr;

        inline virtual ~_Impl_base() = default;

        virtual void _M_run() = 0;
    };

    template <typename _Callable>
    struct _Impl : public _Impl_base {
        _Callable m_func;

        _Impl( _Callable&& __f )
        : m_func( std::forward<_Callable>( __f ) ) {}

        void _M_run() override {
            m_func();
        }
    };

private:
    ThreadId m_threadId;
    std::string m_name;

public:
    Thread() noexcept = default;

    Thread( Thread& ) = delete;
    Thread( const Thread& ) = delete;

    Thread( Thread&& __t ) noexcept {
        swap( __t );
    }

    template <typename _Callable, typename... _Args>
    explicit Thread( const char* taskName, const std::size_t stackSize, _Callable&& __f, _Args&&... __args ) {
        _M_start_thread(
            taskName,
            stackSize,
            _M_make_routine( std::bind( std::forward<_Callable>( __f ), std::forward<_Args>( __args )... ) ) );
    }

    ~Thread() {
        if( joinable() ) {
            std::terminate();
        }
    }

    Thread& operator=( const Thread& ) = delete;

    Thread& operator=( Thread&& __t ) noexcept {
        if( joinable() ) {
            std::terminate();
        }
        swap( __t );
        return *this;
    }

    inline void swap( Thread& __t ) noexcept {
        std::swap( m_threadId, __t.m_threadId );
        std::swap( m_name, __t.m_name );
    }

    inline bool joinable() const noexcept {
        return !( m_threadId == ThreadId() );
    }

    inline operator bool() const noexcept {
        return joinable();
    }

    void join();

    void detach() {
        m_threadId = ThreadId();
    }

    inline auto get_id() const noexcept {
        return m_threadId;
    }

    inline native_handle_type native_handle() {
        return m_threadId.m_threadHandle;
    }

    inline const auto& name() const {
        return m_name;
    }

    // Returns a value that hints at the number of hardware Thread contexts.
    static inline unsigned int hardware_concurrency() noexcept;

private:
    void _M_start_thread( const char* taskName, const std::size_t stackSize, __shared_base_type _impl );

    template <typename _Callable>
    inline std::shared_ptr<_Impl<_Callable>> _M_make_routine( _Callable&& __f ) {
        // Create and allocate full data structure, not base.
        return std::make_shared<_Impl<_Callable>>( std::forward<_Callable>( __f ) );
    }
};  // class Thread

//-----------------------------------------------------------------------------------------------

inline void swap( Thread& __x, Thread& __y ) noexcept {
    __x.swap( __y );
}

inline bool operator!=( Thread::ThreadId __x, Thread::ThreadId __y ) noexcept {
    return !( __x == __y );
}

inline bool operator<=( Thread::ThreadId __x, Thread::ThreadId __y ) noexcept {
    return !( __y < __x );
}

inline bool operator>( Thread::ThreadId __x, Thread::ThreadId __y ) noexcept {
    return __y < __x;
}

inline bool operator>=( Thread::ThreadId __x, Thread::ThreadId __y ) noexcept {
    return !( __x < __y );
}

// DR 889.
/// std::hash specialization for Thread::id.
template <>
struct hash<Thread::ThreadId> : public std::__hash_base<size_t, Thread::ThreadId> {
    size_t operator()( const Thread::ThreadId& __id ) const noexcept {
        return std::_Hash_impl::hash( __id.m_threadHandle );
    }
};

template <class _CharT, class _Traits>
inline std::basic_ostream<_CharT, _Traits>& operator<<( std::basic_ostream<_CharT, _Traits>& __out,
                                                        Thread::ThreadId __id ) {
    if( __id == Thread::ThreadId() )
        return __out << "Thread::id of a non-executing Thread";
    else
        return __out << __id.m_threadHandle;
}

namespace this_thread {

/// get_id
inline Thread::ThreadId get_id() noexcept {
    return Thread::ThreadId( xTaskGetCurrentTaskHandle() );
}

inline void yield() noexcept {
    vTaskDelay( 1 );
}

inline void __sleep_for( const std::chrono::milliseconds& ms ) noexcept {
    vTaskDelay( ms.count() / portTICK_PERIOD_MS );
}

template <typename _Rep, typename _Period>
inline void sleep_for( const std::chrono::duration<_Rep, _Period>& __rtime ) {
    if( __rtime > __rtime.zero() ) {
        const auto __ms = std::chrono::duration_cast<std::chrono::milliseconds>( __rtime );
        __sleep_for( __ms );
    }
}

template <typename _Clock, typename _Duration>
inline void sleep_until( const std::chrono::time_point<_Clock, _Duration>& __atime ) {
    auto __now = _Clock::now();
    if( __now < __atime ) {
        const auto __ms = std::chrono::duration_cast<std::chrono::milliseconds>( __atime - __now );
        __sleep_for( __ms );
    }
}

}  // namespace this_thread

}  // namespace core
