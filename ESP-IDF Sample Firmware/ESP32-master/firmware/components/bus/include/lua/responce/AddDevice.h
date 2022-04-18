#pragma once

#include <DeviceInfo.h>
#include <BusCommand.h>
#include <memory>

namespace lua {
namespace responce {

struct AddDevice final : public bus::IBusCommand {
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::unique_ptr<gateways::BaseDeviceInfo> m_deviceInfo;
};

}  // namespace responce
}  // namespace lua
