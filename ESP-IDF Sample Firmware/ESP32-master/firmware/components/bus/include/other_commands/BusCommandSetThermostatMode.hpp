#pragma once

#include <BusCommand.h>

namespace bus::command {

class SetThermostatMode : public IBusCommand {
public:
    SetThermostatMode( znet_node_id_t nodeId,
                       znet_node_channel_id_t channelId,
                       uint8_t mode,
                       std::optional<std::string> manufacturerSpecificData )
    : m_nodeId( nodeId )
    , m_channelId( channelId )
    , m_mode( mode )
    , m_manufacturerSpecificData( std::move( manufacturerSpecificData ) ) {}

    auto nodeId() const {
        return m_nodeId;
    }

    auto channelId() const {
        return m_channelId;
    }

    auto mode() const {
        return m_mode;
    }

    const auto& manufacturerSpecificData() const {
        return m_manufacturerSpecificData;
    }

    void accept( BusAddr from, BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    znet_node_id_t m_nodeId;
    znet_node_channel_id_t m_channelId;
    uint8_t m_mode;
    std::optional<std::string> m_manufacturerSpecificData;
};

}  // namespace bus::command