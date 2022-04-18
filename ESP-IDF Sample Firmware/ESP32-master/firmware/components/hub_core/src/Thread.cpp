#include "Thread.h"

#include <set>
#include <mutex>

#include "soc/soc.h"

#include "DebugUtils.h"

namespace core {

namespace {

std::mutex g_mutex;
std::set<Thread::native_handle_type> m_activeThreads;

void taskFunction( Thread::_Impl_base* _impl ) {
    {
        _impl->_M_run();
        _impl->_M_this_ptr.reset();

        const auto taskHandle = xTaskGetCurrentTaskHandle();
        std::lock_guard<decltype( g_mutex )> guard( g_mutex );
        m_activeThreads.erase( taskHandle );
    }

    vTaskDelete( NULL );
}
}  // namespace

void Thread::_M_start_thread( const char* taskName, const std::size_t stackSize, __shared_base_type _impl ) {
    m_name = taskName;

    BaseType_t taskResult = xTaskCreate( reinterpret_cast<TaskFunction_t>( taskFunction ),
                                         taskName,
                                         stackSize,
                                         _impl.get(),
                                         10,
                                         &m_threadId.m_threadHandle );

    if( pdPASS == taskResult ) {
        _impl->_M_this_ptr = _impl;
        std::lock_guard<decltype( g_mutex )> guard( g_mutex );
        m_activeThreads.insert( m_threadId.m_threadHandle );
    }
    else {
        FUNC_LOGE( "Failed to create %s task.", taskName );
    }
}

void Thread::join() {
    while( true ) {
        {
            std::lock_guard<decltype( g_mutex )> guard( g_mutex );
            if( m_activeThreads.cend() == m_activeThreads.find( m_threadId.m_threadHandle ) ) {
                break;
            }
        }
        this_thread::yield();
    }

    m_threadId = ThreadId();
}

}  // namespace core