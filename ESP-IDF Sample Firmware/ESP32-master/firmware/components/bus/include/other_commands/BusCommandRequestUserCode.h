#pragma once

#include <BusCommand.h>
#include <string.h>
#include <memory>

namespace bus {
namespace command {

struct RequestUserCode final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    uint8_t m_nodeId;
    uint8_t m_channelId;
    uint8_t m_user_id;
};

}  // namespace command
}  // namespace bus
