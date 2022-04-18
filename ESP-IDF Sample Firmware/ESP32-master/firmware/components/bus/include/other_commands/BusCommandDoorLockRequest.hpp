#pragma once

#include "BusCommand.h"

namespace bus::command {

class DoorLockRequest : public ::bus::IBusCommand {
public:
    DoorLockRequest( uint8_t nodeId, uint8_t channelId )
    : m_nodeId( nodeId )
    , m_channelId( channelId ) {}

    auto getNodeId() const {
        return m_nodeId;
    }

    auto getChannelId() const {
        return m_channelId;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
};

}  // namespace bus::command
