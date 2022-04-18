#pragma once

#include <IValue.h>
#include <znet/znet_notification.hpp>
#include <stdint.h>
#include <vector>

namespace common {
// TODO: Change class to remove dependancy on ZNet library
class NotificationValue final : public IValue {
public:
    using UPtr = std::unique_ptr<NotificationValue>;
    using BlobValueType = uint8_t;
    using BlobType = std::vector<BlobValueType>;

    NotificationValue() = default;

    NotificationValue( const NotificationValue& ) = default;

    NotificationValue( uint8_t v1AlarmType,
                       uint8_t v1AlarmLevel,
                       znet::NotificationType notification,
                       znet::NotificationEvent event,
                       const uint8_t* data,
                       uint8_t dataSize );

    NotificationValue( uint8_t v1AlarmType,
                       uint8_t v1AlarmLevel,
                       znet::NotificationType notification,
                       znet::NotificationEvent event,
                       BlobType&& data );

    znet::NotificationType getType() const {
        return m_notification;
    }

    znet::NotificationEvent getEvent() const {
        return m_event;
    }

    const BlobType& getData() const {
        return m_data;
    }

    uint8_t getAlarmType() const {
        return m_v1AlarmType;
    }

    uint8_t getAlarmLevel() const {
        return m_v1AlarmLevel;
    }

    void accept( IValueVisitorConst& visitor ) const override;

    void accept( IValueVisitor& visitor ) override;

private:
    uint8_t m_v1AlarmType;
    uint8_t m_v1AlarmLevel;
    znet::NotificationType m_notification;
    znet::NotificationEvent m_event;
    BlobType m_data;
};

}  // namespace common
