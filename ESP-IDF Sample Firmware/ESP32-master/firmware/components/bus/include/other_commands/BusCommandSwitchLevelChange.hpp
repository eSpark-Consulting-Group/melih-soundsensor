#pragma once

#include "BusCommand.h"

#include <znet/znet.h>

namespace bus::command {

class SwitchLevelChange : public ::bus::IBusCommand {
public:
    SwitchLevelChange( znet_node_id_t nodeId,
                       uint8_t channelId,
                       std::string command,
                       std::optional<std::chrono::seconds> duration )
    : m_nodeId{ nodeId }
    , m_channelId{ channelId }
    , m_command{ std::move( command ) }
    , m_duration{ duration } {}

    auto getNodeId() const {
        return m_nodeId;
    }

    auto getChannelId() const {
        return m_channelId;
    }

    auto getCommand() const {
        return m_command;
    }

    auto getDuration() const {
        return m_duration;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    znet_node_id_t m_nodeId;
    uint8_t m_channelId;
    std::string m_command;
    std::optional<std::chrono::seconds> m_duration;
};

class ColorSwitchLevelChange : public SwitchLevelChange {
public:
    ColorSwitchLevelChange( znet_node_id_t nodeId,
                            uint8_t channelId,
                            std::string command,
                            std::string colorComponent,
                            std::optional<std::chrono::seconds> duration )
    : SwitchLevelChange{ nodeId, channelId, std::move( command ), std::move( duration ) }
    , m_colorComponent( std::move( colorComponent ) ) {}

    auto getColorComponent() const {
        return m_colorComponent;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    std::string m_colorComponent;
};

}  // namespace bus::command
