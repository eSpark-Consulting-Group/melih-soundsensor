#pragma once

#include <string>
#include <BusCommand.h>

namespace bus {
namespace command {

struct ModesDisarmedDevicesRemoved final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }

    std::string m_modeId;
    std::string m_disarmedDevice;
};

}  // namespace command
}  // namespace bus
