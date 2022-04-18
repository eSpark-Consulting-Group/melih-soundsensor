#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

class LockAntiTheftDefault final : public bus::IBusCommand {
public:
    LockAntiTheftDefault( uint8_t nodeId, uint8_t channelId )
    : m_nodeId( nodeId )
    , m_channelId( channelId ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline uint8_t nodeId() const {
        return m_nodeId;
    }

    inline uint8_t channelId() const {
        return m_channelId;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
};

}  // namespace command
}  // namespace bus
