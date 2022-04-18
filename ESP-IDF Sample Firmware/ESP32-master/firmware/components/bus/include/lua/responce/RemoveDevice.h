#pragma once

#include <string>
#include <BusCommand.h>

namespace lua {
namespace responce {

struct RemoveDevice final : public bus::IBusCommand {
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }

    std::string m_deviceId;
    bool m_isServiceNotification = true;
};

}  // namespace responce
}  // namespace lua
