#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

class RequestBarrierSignal final : public bus::IBusCommand {
public:
    RequestBarrierSignal( uint8_t nodeId, uint8_t channelId, std::string signalName )
    : m_nodeId( nodeId )
    , m_channelId( channelId )
    , m_signal_name( signalName ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline uint8_t nodeId() const {
        return m_nodeId;
    }

    inline uint8_t channelId() const {
        return m_channelId;
    }

    inline std::string signalName() const {
        return m_signal_name;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
    std::string m_signal_name;
};

}  // namespace command
}  // namespace bus
