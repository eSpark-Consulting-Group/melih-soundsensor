#pragma once

#include <string>
#include <IValue.h>
#include <BusCommand.h>
#include "DeviceSettingsInfo.hpp"

namespace settings {
namespace responces {

class SettingDictionaryValueUpdated final : public bus::IBusCommand {
public:
    SettingDictionaryValueUpdated( std::unique_ptr<::gateways::DeviceSettingsInfo> settingInfo,
                                   std::unique_ptr<::common::IValue> value,
                                   std::string key,
                                   std::string operation )
    : m_settingInfo( std::move( settingInfo ) )
    , m_value( std::move( value ) )
    , m_key( std::move( key ) )
    , m_operation( std::move( operation ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    auto getSettingInfo() const {
        return m_settingInfo.get();
    }

    auto getValue() const {
        return m_value.get();
    }

    auto getKey() const {
        return m_key;
    }

    auto getOperation() const {
        return m_operation;
    }

private:
    std::unique_ptr<gateways::DeviceSettingsInfo> m_settingInfo;
    common::IValue::UPtr m_value;
    std::string m_key;
    std::string m_operation;
};

}  // namespace responces
}  // namespace settings
