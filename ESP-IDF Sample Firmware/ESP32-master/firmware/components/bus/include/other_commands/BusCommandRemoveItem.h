#pragma once

#include <string>
#include <BusCommand.h>

namespace bus {
namespace command {

struct RemoveItem final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_itemId;
    std::string m_deviceId;
};

}  // namespace command
}  // namespace bus
