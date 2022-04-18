#pragma once

#include <string>
#include <BusCommand.h>
#include <DeviceInfo.h>

namespace settings {
namespace responces {

class SettingRemoved final : public bus::IBusCommand {
public:
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_settingId;
};

}  // namespace responces
}  // namespace settings
