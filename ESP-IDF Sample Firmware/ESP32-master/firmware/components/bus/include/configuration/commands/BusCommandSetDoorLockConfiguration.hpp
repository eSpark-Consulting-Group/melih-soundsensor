#pragma once

#include <optional>

#include "BusCommand.h"
#include "configuration/ExtDoorLockConfiguration.h"

namespace configuration::commands {

class SetDoorLockConfiguration : public ::bus::IBusCommand {
public:
    SetDoorLockConfiguration( uint8_t nodeId,
                              uint8_t channelId,
                              std::vector<uint8_t> insideHandleId,
                              std::vector<uint8_t> outsideHandleId,
                              std::optional<uint16_t> lockTimeout,
                              std::optional<ExtDoorLockConfiguration> extConfiguration )
    : m_nodeId( nodeId )
    , m_channelId( channelId )
    , m_insideHandleId( insideHandleId )
    , m_outsideHandleId( outsideHandleId )
    , m_lockTimeout( lockTimeout )
    , m_extConfiguration( extConfiguration ) {}

    auto getNodeId() const {
        return m_nodeId;
    }

    auto getChannelId() const {
        return m_channelId;
    }

    const auto& getInsideHandleIds() const {
        return m_insideHandleId;
    }

    const auto& getOutsideHandleIds() const {
        return m_outsideHandleId;
    }

    auto getLockTimeout() const {
        return m_lockTimeout;
    }

    auto getExtendedConfiguration() const {
        return m_extConfiguration;
    }

    void accept(::bus::BusAddr from, ::bus::BusAddr to, ::bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    uint8_t m_nodeId;
    uint8_t m_channelId;
    std::vector<uint8_t> m_insideHandleId;
    std::vector<uint8_t> m_outsideHandleId;
    std::optional<uint16_t> m_lockTimeout;
    std::optional<ExtDoorLockConfiguration> m_extConfiguration;
};

}  // namespace configuration::commands