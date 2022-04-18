#pragma once

#include <string>
#include <BusCommand.h>
#include <DeviceInfo.h>

namespace settings {
namespace responces {

class SettingStatusUpdated final : public bus::IBusCommand {
public:
    SettingStatusUpdated( std::string settingId, std::string status )
    : m_settingId( std::move( settingId ) )
    , m_status( std::move( status ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline const std::string& getStatus() {
        return m_status;
    }

    inline const std::string& getSettingId() {
        return m_settingId;
    }

private:
    std::string m_settingId;
    std::string m_status;
};

}  // namespace responces
}  // namespace settings
