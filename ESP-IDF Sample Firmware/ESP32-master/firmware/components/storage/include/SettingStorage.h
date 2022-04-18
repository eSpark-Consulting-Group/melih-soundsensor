#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace common {
class IValue;
}
namespace gateways {
class DeviceSettingsInfo;
class GatewaySettingsInfo;
}  // namespace gateways

namespace storage {

class KeyValueStorageFactory;

class Settings {
public:
    using SettingStatus = std::string;

    Settings( const Settings& ) = delete;
    Settings& operator=( const Settings& ) = delete;

    bool setStatus( const std::string& settingId, SettingStatus newStatus );
    std::optional<SettingStatus> getStatus( const std::string& settingId );

    bool setValue( const std::string& settingId, std::unique_ptr<::common::IValue> value );
    std::unique_ptr<::common::IValue> getValue( const std::string& settingId );

    void remove( const std::string& settingId );

    static void restoreLegacyValuesFromCache();

protected:
    Settings( KeyValueStorageFactory& storageFactory, const char* storageName );

    template <typename T>
    std::unique_ptr<T> getInfo( const std::string& settingId );

    template <typename T>
    bool setInfo( const T& settingsInfo, SettingStatus newStatus, std::unique_ptr<::common::IValue> value );

    template <typename T>
    void enumerate( const std::function<bool( const T& )>& cb );

    KeyValueStorageFactory& m_factory;
    const char* const m_storage_name;
};

class DeviceSettings : public Settings {
public:
    static DeviceSettings& get();

    bool setInfo( const gateways::DeviceSettingsInfo&,
                  SettingStatus newStatus,
                  std::unique_ptr<::common::IValue> value );

    std::unique_ptr<gateways::DeviceSettingsInfo> getInfo( const std::string& settingId );

    void enumerate( const std::function<bool( const gateways::DeviceSettingsInfo& )>& cb );

private:
    using Settings::Settings;
};

class GatewaySettings : public Settings {
public:
    static GatewaySettings& get();

    bool setInfo( const gateways::GatewaySettingsInfo&,
                  SettingStatus newStatus,
                  std::unique_ptr<::common::IValue> value );
    std::unique_ptr<gateways::GatewaySettingsInfo> getInfo( const std::string& settingId );

    void enumerate( const std::function<bool( const gateways::GatewaySettingsInfo& )>& cb );

private:
    using Settings::Settings;
};

}  // namespace storage
