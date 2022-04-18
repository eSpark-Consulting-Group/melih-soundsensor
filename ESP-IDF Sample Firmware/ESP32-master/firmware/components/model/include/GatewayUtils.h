#pragma once

#include <GatewayDefs.h>
#include <GatewayID.h>
#include <memory>
#include <vector>

namespace common {
class JsonStreamGenerator;
class IValue;
class TableValue;
}  // namespace common

namespace znet {
struct ProvisioningListEntryFields;
}

namespace gateways {

class BaseDeviceInfo;
class ItemInfo;
class SettingsInfo;
class DeviceSettingsInfo;
class GatewaySettingsInfo;

namespace utils {

std::unique_ptr<::common::TableValue> loadGatewayUserDataFromConfigJson( GatewayID );
void convertGatewayUserDataFieldNamesToLuaNames(::common::TableValue& );
void convertGatewayUserDataFieldNamesFromLuaNames(::common::TableValue& );

std::string getPluginNameByItemId( const char* itemId );

std::string getPluginNameBySettingId( const char* settingId );

std::string getPluginNameByDeviceId( const char* deviceId );

bool fillDevicePath( char* resultBuffer, uint16_t maxBuffferSize, char gatewayPrefix, const DeviceId& deviceId );

bool fillDevicePath( char* resultBuffer, uint16_t maxBufferSize, char gatewayPrefix, std::string deviceId );

bool extractDeviceIdFromPublicId( const char* publicDeviceId, GatewayID& gatewayId, DeviceId& deviceId );

bool extractItemIdFromPublicId( const char* publicItemId, GatewayID& gatewayId, DeviceId& deviceId, ItemId& itemIdx );

bool writeDeviceInfo( common::JsonStreamGenerator& jsonGenerator, const BaseDeviceInfo& deviceInfo );

bool writeItemInfo( common::JsonStreamGenerator& jsonGenerator, const ItemInfo& itemInfo );
bool writeGatewaySettingInfo( common::JsonStreamGenerator& jsonGenerator, const GatewaySettingsInfo& settingInfo );

bool writeSettingInfo( common::JsonStreamGenerator& jsonGenerator, const DeviceSettingsInfo& settingInfo );

bool writeDeviceInfoWithFilter( common::JsonStreamGenerator& jsonGenerator,
                                const BaseDeviceInfo& deviceInfo,
                                std::vector<std::string> filter );

bool writeItemInfoWithFilter( common::JsonStreamGenerator& jsonGenerator,
                              const ItemInfo& itemInfo,
                              std::vector<std::string> filter );

bool writeUpdatedField( common::JsonStreamGenerator& jsonGenerator,
                        const char* id,
                        const char* fieldId,
                        const common::IValue* fieldValue );

bool writeItemUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                           const ItemInfo& itemInfo,
                           common::IValue* value );

bool writeItemDictionaryUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                                     const ItemInfo& itemInfo,
                                     const std::string operation,
                                     const std::string itemDictionaryId,
                                     common::IValue* value );

bool writeSettingUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                              const SettingsInfo& settingInfo,
                              common::IValue* value );

bool writeSettingUpdatedInfo( common::JsonStreamGenerator& jsonGenerator, common::IValue* value );

bool writeGatewaySettingsUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                                      const GatewaySettingsInfo& settingInfo,
                                      const common::IValue* value );

bool writeSettingStatusUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                                    const std::string& settingInfo,
                                    const std::string& status );

bool writeSettingDictionaryUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                                        const DeviceSettingsInfo& settingInfo,
                                        const std::string operation,
                                        const std::string key,
                                        const common::IValue* value );

bool writeProvisioningListEntryFields(::common::JsonStreamGenerator&, const ::znet::ProvisioningListEntryFields& );

bool writeGatewaySettingsInfo( common::JsonStreamGenerator& jsonGenerator, const GatewaySettingsInfo& settingInfo );

}  // namespace utils
}  // namespace gateways
