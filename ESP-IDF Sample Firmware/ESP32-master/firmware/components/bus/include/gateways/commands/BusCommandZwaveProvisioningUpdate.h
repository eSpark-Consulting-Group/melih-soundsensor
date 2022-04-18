#pragma once

#include <optional>

#include "BusCommand.h"

namespace gateways::commands {

class ZwaveProvisioningUpdate final : public ::bus::IBusCommand {
public:
    ZwaveProvisioningUpdate( uint8_t nodeId, std::optional<std::string> name, std::optional<std::string> room )
    : m_nodeId( nodeId )
    , m_name( std::move( name ) )
    , m_room( std::move( room ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    auto getNodeId() const {
        return m_nodeId;
    }

    auto getName() const {
        return m_name;
    }

    auto getRoom() const {
        return m_room;
    }

private:
    uint8_t m_nodeId;
    std::optional<std::string> m_name;
    std::optional<std::string> m_room;
};

}  // namespace gateways::commands
