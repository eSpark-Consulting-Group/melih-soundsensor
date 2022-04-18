#pragma once

#include "BusCommand.h"

namespace configuration::commands {

class ResetAllConfigurationValues : public ::bus::IBusCommand {
public:
    explicit ResetAllConfigurationValues( uint8_t nodeId )
    : m_nodeId( nodeId ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    auto getNodeId() const {
        return m_nodeId;
    }

private:
    uint8_t m_nodeId;
};

}  // namespace configuration::commands
