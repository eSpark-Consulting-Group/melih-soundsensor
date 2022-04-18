#pragma once

#include "BusCommand.h"

namespace bus::command {

class RequestAlarmV1Value : public IBusCommand {
public:
    RequestAlarmV1Value( znet_node_id_t nodeId, znet_node_channel_id_t channelId, uint8_t notificationType )
    : m_nodeId{ nodeId }
    , m_channelId{ channelId }
    , m_notificationType{ notificationType } {}

    auto nodeId() const {
        return m_nodeId;
    }

    auto channelId() const {
        return m_channelId;
    }

    auto alarmType() const {
        return m_notificationType;
    }

    void accept(::bus::BusAddr from, ::bus::BusAddr to, ::bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    znet_node_id_t m_nodeId;
    znet_node_channel_id_t m_channelId;
    uint8_t m_notificationType;
};

}  // namespace bus::command
