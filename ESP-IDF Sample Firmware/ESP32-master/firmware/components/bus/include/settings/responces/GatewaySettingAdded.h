#pragma once

#include <string>
#include <BusCommand.h>
#include <SettingsInfo.h>

namespace settings {
namespace responces {

class GatewaySettingAdded final : public bus::IBusCommand {
public:
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::unique_ptr<gateways::GatewaySettingsInfo> m_settingInfo;
};

}  // namespace responces
}  // namespace settings
