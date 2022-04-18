#pragma once

#include <string>
#include <IValue.h>
#include <BusCommand.h>
#include <DeviceInfo.h>
#include "DeviceSettingsInfo.hpp"

namespace settings {
namespace responces {

class SettingUpdated final : public bus::IBusCommand {
public:
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    };

    common::IValue::UPtr m_setting;
};

}  // namespace responces
}  // namespace settings
