#pragma once

#include <string>
#include <BusCommand.h>

namespace bus {
namespace command {

struct ModesDisarmedDevicesDefault final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_modeId;
    bool m_disarmedDefault;
};

}  // namespace command
}  // namespace bus
