#pragma once

#include <BusCommand.h>
#include <IValue.h>
#include <string.h>
#include <memory>

namespace bus {
namespace command {

struct SetValue final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    uint8_t m_nodeId;
    uint8_t m_commandClassId;
    uint8_t m_channelId;
    common::IValue::UPtr m_value;
};

}  // namespace command
}  // namespace bus
