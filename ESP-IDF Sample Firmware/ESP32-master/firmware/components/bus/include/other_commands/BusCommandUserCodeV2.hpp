#pragma once

#include <znet/znet.h>

#include "BusCommand.h"

namespace bus::command {

class UserCodeV2Operation : public IBusCommand {
public:
    enum RequestType { REQUEST_MASTER_CODE, REQUEST_KEYPAD_MODE, REQUEST_CHECKSUM, SET_MASTER_CODE, SET_KEYPAD_MODE };

    UserCodeV2Operation( RequestType type,
                         znet_node_id_t nodeId,
                         uint8_t channelId,
                         std::optional<std::string> arg = std::nullopt )
    : m_type{ type }
    , m_nodeId{ nodeId }
    , m_channelId{ channelId }
    , m_arg{ std::move( arg ) } {}

    auto getType() const {
        return m_type;
    }

    auto getNodeId() const {
        return m_nodeId;
    }

    auto getChannelId() const {
        return m_channelId;
    }

    auto getArg() const {
        return m_arg;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    RequestType m_type;
    znet_node_id_t m_nodeId;
    uint8_t m_channelId;
    std::optional<std::string> m_arg;
};

}  // namespace bus::command
