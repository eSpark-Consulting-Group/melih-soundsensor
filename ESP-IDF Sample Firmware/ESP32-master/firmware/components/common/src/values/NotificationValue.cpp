#include <values/NotificationValue.h>
#include <znet/znet.hpp>
#include <IValueVisitor.h>

namespace common {

NotificationValue::NotificationValue( uint8_t v1AlarmType,
                                      uint8_t v1AlarmLevel,
                                      znet::NotificationType notification,
                                      znet::NotificationEvent event,
                                      const uint8_t* data,
                                      uint8_t dataSize )
: m_v1AlarmType( v1AlarmType )
, m_v1AlarmLevel( v1AlarmLevel )
, m_notification( notification )
, m_event( event )
, m_data( &data[0u], &data[dataSize] ) {}

NotificationValue::NotificationValue( uint8_t v1AlarmType,
                                      uint8_t v1AlarmLevel,
                                      znet::NotificationType notification,
                                      znet::NotificationEvent event,
                                      BlobType&& data )
: m_v1AlarmType( v1AlarmType )
, m_v1AlarmLevel( v1AlarmLevel )
, m_notification( notification )
, m_event( event )
, m_data( std::move( data ) ) {}

void NotificationValue::accept( IValueVisitorConst& visitor ) const {
    visitor.visit( *this );
}

void NotificationValue::accept( IValueVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace common
