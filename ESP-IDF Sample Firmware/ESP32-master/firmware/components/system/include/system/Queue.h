

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <type_traits>
#include <optional>
#include <chrono>

namespace sys {

class FreeRtosQueueVoidPtr {
public:
    static constexpr auto ValueTypeSize = sizeof( void* );

    FreeRtosQueueVoidPtr( const unsigned int queueSize = 32 )
    : m_queue{ xQueueCreate( queueSize, ValueTypeSize ) } {}

    FreeRtosQueueVoidPtr( const FreeRtosQueueVoidPtr& ) = delete;

    ~FreeRtosQueueVoidPtr() {
        if( m_queue ) {
            vQueueDelete( m_queue );
        }
    }

    inline bool send( const void* value ) {
        return pdTRUE == xQueueSend( m_queue, &value, 0 );
    }

    inline bool send( const void* value, const std::chrono::milliseconds timeout ) {
        return pdTRUE == xQueueSend( m_queue, &value, pdMS_TO_TICKS( timeout.count() ) );
    }

    inline void* recv() {
        void* value;
        return ( pdTRUE == xQueueReceive( m_queue, &value, 0 ) ) ? value : nullptr;
    }

    inline void* recv( const std::chrono::milliseconds timeout ) {
        void* value;
        return ( pdTRUE == xQueueReceive( m_queue, &value, pdMS_TO_TICKS( timeout.count() ) ) ) ? value : nullptr;
    }

    inline bool isValid() const {
        return NULL != m_queue;
    }

    inline std::size_t size() const {
        return uxQueueMessagesWaiting( m_queue );
    }

protected:
    QueueHandle_t m_queue{ NULL };
};

template <typename ValueType, typename = typename std::enable_if<std::is_pointer<ValueType>::value, ValueType>::type>
class FreeRtosQueuePtr : public FreeRtosQueueVoidPtr {
public:
    using FreeRtosQueueVoidPtr::FreeRtosQueueVoidPtr;

    inline bool send( const ValueType value ) {
        return FreeRtosQueueVoidPtr::send( static_cast<void*>( value ) );
    }

    inline bool send( const ValueType value, const std::chrono::milliseconds timeout ) {
        return FreeRtosQueueVoidPtr::send( static_cast<void*>( value ), timeout );
    }

    inline ValueType recv() {
        return static_cast<ValueType>( FreeRtosQueueVoidPtr::recv() );
    }

    inline ValueType recv( const std::chrono::milliseconds timeout ) {
        return static_cast<ValueType>( FreeRtosQueueVoidPtr::recv( timeout ) );
    }
};

template <typename ValueType,
          typename = typename std::enable_if<std::is_trivially_copyable<ValueType>::value, ValueType>::type>
class FreeRtosQueuePod {
public:
    static constexpr auto ValueTypeSize = sizeof( ValueType );

    FreeRtosQueuePod( const unsigned int queueSize = 32 )
    : m_queue{ xQueueCreate( queueSize, ValueTypeSize ) } {}

    FreeRtosQueuePod( const FreeRtosQueuePod& ) = delete;

    ~FreeRtosQueuePod() {
        if( m_queue ) {
            vQueueDelete( m_queue );
        }
    }

    inline bool send( const ValueType value ) {
        return pdTRUE == xQueueSend( m_queue, &value, 0 );
    }

    inline bool send( const ValueType value, const std::chrono::milliseconds timeout ) {
        return pdTRUE == xQueueSend( m_queue, &value, pdMS_TO_TICKS( timeout.count() ) );
    }

    inline std::optional<ValueType> recv() {
        ValueType value;
        return ( pdTRUE == xQueueReceive( m_queue, &value, 0 ) ) ? std::make_optional( value ) : std::nullopt;
    }

    inline std::optional<ValueType> recv( const std::chrono::milliseconds timeout ) {
        ValueType value;
        return ( pdTRUE == xQueueReceive( m_queue, &value, pdMS_TO_TICKS( timeout.count() ) ) ) ?
                   std::make_optional( value ) :
                   std::nullopt;
    }

    inline bool isValid() const {
        return NULL != m_queue;
    }

    inline std::size_t size() const {
        return uxQueueMessagesWaiting( m_queue );
    }

private:
    QueueHandle_t m_queue{ NULL };
};

}  // namespace sys
