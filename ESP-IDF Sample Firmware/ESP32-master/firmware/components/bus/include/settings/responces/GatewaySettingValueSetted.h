#pragma once

#include <string>
#include <IValue.h>
#include <BusCommand.h>
#include "GatewaySettingsInfo.hpp"

namespace settings {
namespace responces {

class GatewaySettingValueSetted final : public bus::IBusCommand {
public:
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::unique_ptr<gateways::GatewaySettingsInfo> m_settingInfo;
    common::IValue::UPtr m_value;
    std::string m_key;
    std::string m_operation;
};

}  // namespace responces
}  // namespace settings
