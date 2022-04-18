#pragma once

#include <BusCommand.h>
#include <string>
#include <IValue.h>
#include <list>

namespace settings {
namespace commands {

class DeviceSettingReset final : public bus::IBusCommand {
public:
    using StringType = std::string;

    DeviceSettingReset( std::string settingId, std::string deviceId )
    : m_settingId( std::move( settingId ) )
    , m_deviceId( std::move( deviceId ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const auto& settingId() const {
        return m_settingId;
    }

    inline const auto& deviceId() const {
        return m_deviceId;
    }

private:
    std::string m_settingId;
    std::string m_deviceId;
};

}  // namespace commands
}  // namespace settings
