#pragma once

#include <string>
#include <BusCommand.h>
#include <DeviceInfo.h>
#include "DeviceSettingsInfo.hpp"

namespace settings {
namespace responces {

class SettingAdded final : public bus::IBusCommand {
public:
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::unique_ptr<gateways::DeviceSettingsInfo> m_settingInfo;
};

}  // namespace responces
}  // namespace settings
