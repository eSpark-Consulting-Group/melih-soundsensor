#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

class SetBarrierSignal final : public bus::IBusCommand {
public:
    SetBarrierSignal( uint8_t nodeId, uint8_t channelId, std::string signalName, uint8_t value )
    : m_nodeId( nodeId )
    , m_channelId( channelId )
    , m_signal_name( signalName )
    , m_value( value ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline uint8_t nodeId() const {
        return m_nodeId;
    }

    inline uint8_t channelId() const {
        return m_channelId;
    }

    inline uint8_t value() const {
        return m_value;
    }

    inline std::string signalName() const {
        return m_signal_name;
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
    std::string m_signal_name;
    uint8_t m_value;
};

}  // namespace command
}  // namespace bus
