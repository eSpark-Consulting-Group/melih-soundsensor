#pragma once

#include <assert.h>
#include <chrono>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace common {
class Mutex {
public:
    using Timeout = std::chrono::milliseconds;

public:
    Mutex()
    : m_semaphore( nullptr ) {
        vSemaphoreCreateBinary( m_semaphore );

        assert( m_semaphore != nullptr );
    }

    ~Mutex() {
        vSemaphoreDelete( m_semaphore );
        m_semaphore = nullptr;
    }

    Mutex( const Mutex& other ) = delete;
    Mutex( const Mutex&& other ) = delete;
    Mutex& operator=( const Mutex& other ) = delete;

    //
    // Note: take() by default will block while getting the semaphore for infinite time
    //
    inline bool take( Timeout timeout = Timeout( 0 ) ) {
        if( timeout == Timeout( 0 ) ) {
            return ( pdPASS == xSemaphoreTake( m_semaphore, portMAX_DELAY ) );
        }
        else {
            return ( pdPASS == xSemaphoreTake( m_semaphore, pdMS_TO_TICKS( timeout.count() ) ) );
        }
    }

    inline bool release() {
        return ( pdPASS == xSemaphoreGive( m_semaphore ) );
    }

private:
    SemaphoreHandle_t m_semaphore;
};
};  // namespace common
