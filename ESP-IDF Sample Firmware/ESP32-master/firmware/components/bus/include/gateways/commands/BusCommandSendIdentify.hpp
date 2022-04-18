#pragma once

#include "BusCommand.h"

namespace bus::command {

/**
 * Sends identify command to Indicator command class.
 */
class SendIdentify : public IBusCommand {
public:
    SendIdentify( uint8_t nodeId, uint8_t channelId )
    : m_nodeId( nodeId )
    , m_channelId( channelId ) {}

    auto getNodeId() const {
        return m_nodeId;
    }

    auto getChannelId() const {
        return m_channelId;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
};

}  // namespace bus::command
