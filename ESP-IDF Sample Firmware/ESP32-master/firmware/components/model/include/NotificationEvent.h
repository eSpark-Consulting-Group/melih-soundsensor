#pragma once

#include <values/NotificationValue.h>
#include <znet/znet_notification.hpp>
#include <cinttypes>

// TODO: Remove dependancy on ZNet library

namespace gateways {

// ItemType notificationToItemType( const znet::NotificationType notification );

struct UINotificationEvent {
    uint8_t value;
    const char* token;
};

UINotificationEvent notificationToUINotificationEvent( const common::NotificationValue notification );

}  // namespace gateways
