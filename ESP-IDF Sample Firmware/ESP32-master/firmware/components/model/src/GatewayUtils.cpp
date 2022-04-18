#include <GatewayUtils.h>
#include <DeviceInfo.h>
#include <DebugUtils.h>
#include <Utils.h>
#include <Storage.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <IGateway.h>
#include <plugin_manager.h>

#include <Volume.h>
#include <algorithm>

#include <string.h>
#include <inttypes.h>
#include <cinttypes>

#include <CommonResources.h>
#include <IValueVisitor.h>
#include <IValueCast.h>
#include <values/IntValue.h>
#include <values/FloatValue.h>
#include <values/ScalebleValue.h>
#include <values/BoolValue.h>
#include <values/StringValue.h>
#include <values/RgbValue.h>
#include <values/NotificationValue.h>
#include <values/ArrayValue.h>
#include <values/TableValue.h>
#include <values/UserCodeValue.h>
#include <NotificationEvent.h>
#include <JsonStreamGenerator.h>
#include <DeviceSettingsManager.h>
#include <Ezlo.h>
#include <ObjectSerializer.h>
#include <HouseModesManager.h>
#include "HubSettings.h"
#include "SettingStorage.h"

#include <converters/ScalesConverterVisitor.hpp>
#include <converters/UiValueFormattedGetterVisitor.hpp>
#include <znet/converters/security_modes_converter.hpp>
#include <znet/znet_provisioning_list_entry.hpp>
#include <SettingsInfo.h>

namespace gateways {
namespace utils {

using namespace common::resources;
namespace lua_tokens = ::common::resources::lua_events;

namespace {

bool writeItemValue( common::JsonStreamGenerator& jsonGenerator, const ItemInfo& itemInfo, common::IValue& value ) {
    bool result = jsonGenerator.writeProperty( tokens::ValueType, toString( itemInfo.getValueType() ) );
    common::ScalesConverterVisitor visitor(
        core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() ),
        common::ScalesConverterVisitor::Precision );
    value.accept( visitor );

    result = result && jsonGenerator.writeProperty( tokens::Value, value );

    common::UiValueFormattedGetterVisitor formattedValueConverter;
    result = result && jsonGenerator.writeProperty( tokens::ValueFormatted, formattedValueConverter.getValue( value ) );

    if( result && itemInfo.isEnum() ) {
        result = jsonGenerator.writePropertyKey( tokens::Enum ) && jsonGenerator.writeOpeningArray();
        const auto enumsCount = itemInfo.getEnumItemsCount();
        for( auto idx = 0; idx < enumsCount && result; ++idx ) {
            auto enumStr = itemInfo.getEnumValue( idx );
            result = jsonGenerator.writeProperty( nullptr, enumStr );
        }
        if( !jsonGenerator.writeClosingSymbol() ) {
            result = false;
        }
    }

    if( result && itemInfo.isMinMax() ) {
        auto minmax = itemInfo.getMinMax();
        result = jsonGenerator.writeProperty( common::resources::tokens::MinValue, minmax.first );
        result = result && jsonGenerator.writeProperty( common::resources::tokens::MaxValue, minmax.second );
    }
    result = result && jsonGenerator.writeProperty( tokens::Status, itemInfo.getStatus() );

    return result;
}

bool writeGatewaySettingValue( common::JsonStreamGenerator& jsonGenerator,
                               const gateways::SettingsInfo& settingInfo,
                               const common::IValue& value ) {
    bool result = jsonGenerator.writeProperty( tokens::ValueType, toString( settingInfo.getValueType() ) );
    if( settingInfo.getElementType() != ItemValueType::NoValue ) {
        result = result && jsonGenerator.writeProperty( tokens::ElementType, toString( settingInfo.getElementType() ) );
    }
    result = result && jsonGenerator.writeProperty( tokens::Value, value );

    if( result && settingInfo.isEnum() ) {
        result = jsonGenerator.writePropertyKey( tokens::Enum ) && jsonGenerator.writeOpeningObject();
        const auto enumsCount = settingInfo.getEnumItemsCount();
        for( auto idx = 0; idx < enumsCount && result; ++idx ) {
            auto enumValue = settingInfo.getEnumValue( idx );
            result = result && jsonGenerator.writePropertyKey( enumValue.first.c_str(), enumValue.first.length() ) &&
                     jsonGenerator.writeOpeningObject();

            if( !enumValue.second.lang_tag.empty() )
                result = result && jsonGenerator.writeProperty( "lang_tag", enumValue.second.lang_tag );
            result = result && jsonGenerator.writeProperty( "text", enumValue.second.text );
            if( !jsonGenerator.writeClosingSymbol() ) {
                result = false;
            }
        }
        if( !jsonGenerator.writeClosingSymbol() ) {
            result = false;
        }
    }

    if( result && settingInfo.isDescription() ) {
        result = jsonGenerator.writePropertyKey( tokens::Description ) && jsonGenerator.writeOpeningObject();
        if( !settingInfo.getDescription().lang_tag.empty() )
            result = result && jsonGenerator.writeProperty( tokens::LangTag, settingInfo.getDescription().lang_tag );
        result = result && jsonGenerator.writeProperty( tokens::Text, settingInfo.getDescription().text );
        if( !jsonGenerator.writeClosingSymbol() ) {
            result = false;
        }
    }
    return result;
}

bool writeSettingValue( common::JsonStreamGenerator& jsonGenerator,
                        const SettingsInfo& settingInfo,
                        common::IValue& value ) {
    bool result = true;

    if( settingInfo.getElementType() != ItemValueType::NoValue ) {
        std::string valueTypeStr( toString( settingInfo.getValueType() ) );
        valueTypeStr += ".";
        valueTypeStr += toString( settingInfo.getElementType() );
        result = result && jsonGenerator.writeProperty( tokens::ValueType, valueTypeStr.c_str() );
        result = result && jsonGenerator.writeProperty( tokens::ElementType, toString( settingInfo.getElementType() ) );
    }
    else {
        result = result && jsonGenerator.writeProperty( tokens::ValueType, toString( settingInfo.getValueType() ) );
    }

    common::ScalesConverterVisitor visitor(
        core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() ),
        common::ScalesConverterVisitor::Precision );
    value.accept( visitor );

    result = result && jsonGenerator.writeProperty( tokens::Value, value );

    if( result && settingInfo.isEnum() ) {
        result = jsonGenerator.writePropertyKey( tokens::Enum ) && jsonGenerator.writeOpeningObject();
        const auto enumsCount = settingInfo.getEnumItemsCount();
        for( auto idx = 0; idx < enumsCount && result; ++idx ) {
            auto enumValue = settingInfo.getEnumValue( idx );
            result = result && jsonGenerator.writePropertyKey( enumValue.first.c_str(), enumValue.first.length() ) &&
                     jsonGenerator.writeOpeningObject();

            if( !enumValue.second.lang_tag.empty() )
                result = result && jsonGenerator.writeProperty( "lang_tag", enumValue.second.lang_tag );
            result = result && jsonGenerator.writeProperty( "text", enumValue.second.text );
            if( !jsonGenerator.writeClosingSymbol() ) {
                result = false;
            }
        }
        if( !jsonGenerator.writeClosingSymbol() ) {
            result = false;
        }
    }

    if( result && settingInfo.isMinMax() ) {
        float valMin, valMax;
        std::tie( valMin, valMax ) = settingInfo.getMinMax();

        {
            auto newVal = settingInfo.castToItemType( std::make_unique<common::FloatValue>( valMin ) );
            if( newVal ) {
                common::ScalesConverterVisitor visitor(
                    ::core::HubSettingsInfo::temperatureScaleToStr(::core::HubSettings::get().getTemperatureScale() ),
                    common::ScalesConverterVisitor::Precision );
                newVal->accept( visitor );
                auto ivalMin = common::iValueCast<common::FloatValue>( std::move( newVal ) );
                if( ivalMin ) {
                    valMin = ivalMin->get();
                }
            }
        }

        {
            auto newVal = settingInfo.castToItemType( std::make_unique<common::FloatValue>( valMax ) );
            if( newVal ) {
                common::ScalesConverterVisitor visitor(
                    ::core::HubSettingsInfo::temperatureScaleToStr(::core::HubSettings::get().getTemperatureScale() ),
                    common::ScalesConverterVisitor::Precision );
                newVal->accept( visitor );
                auto ivalMax = common::iValueCast<common::FloatValue>( std::move( newVal ) );
                if( ivalMax ) {
                    valMax = ivalMax->get();
                }
            }
        }

        result = result && jsonGenerator.writeProperty( tokens::ValueMin, valMin );
        result = result && jsonGenerator.writeProperty( tokens::ValueMax, valMax );
    }

    if( result && settingInfo.isDescription() ) {
        result = jsonGenerator.writePropertyKey( tokens::Description ) && jsonGenerator.writeOpeningObject();
        if( !settingInfo.getDescription().lang_tag.empty() )
            result = result && jsonGenerator.writeProperty( tokens::LangTag, settingInfo.getDescription().lang_tag );
        result = result && jsonGenerator.writeProperty( tokens::Text, settingInfo.getDescription().text );
        if( !jsonGenerator.writeClosingSymbol() ) {
            result = false;
        }
    }
    return result;
}

}  // namespace

std::unique_ptr<::common::TableValue> loadGatewayUserDataFromConfigJson( GatewayID gatewayId ) {
    auto config = ::plugin::PluginManager::get().getPluginConfig( toString( gatewayId ) );
    if( !config.json ) {
        FUNC_LOGE( "Cannot load config.json for plugin %s", ::gateways::toString( gatewayId ) );
        return {};
    }
    auto gatewayJson = config.json( tokens::Gateway );
    if( !gatewayJson ) {
        return {};
    }
    auto gatewayData = std::make_unique<::common::TableValue>();
    gatewayData->insert( tokens::Settings, std::make_unique<::common::ArrayValue>() );
    gatewayData->insert( tokens::Id, ::common::IValue::create( toString( gatewayId ) ) );
    gatewayData->insert( tokens::Ready, ::common::IValue::create( true ) );
    gatewayData->insert( tokens::Status, ::common::IValue::create( tokens::Ready ) );
    for( auto fieldName :
         { tokens::Name, tokens::Label, tokens::Reason, tokens::UnreachableReasons, tokens::UnreachableActions } ) {
        if( auto fieldNode = gatewayJson( fieldName ) ) {
            gatewayData->insert( fieldName, ::common::makeIval( fieldNode ) );
        }
    }
    if( auto pluginId = config.json( tokens::Id ) ) {
        gatewayData->insert( tokens::PluginId, ::common::makeIval( pluginId ) );
    }
    return gatewayData;
}

void convertGatewayUserDataFieldNamesToLuaNames(::common::TableValue& fields ) {
    static std::initializer_list<std::pair<const char*, const char*>> replaceTable = {
        { tokens::UnreachableReasons, lua_tokens::UnreachableReasons },
        { tokens::UnreachableActions, lua_tokens::UnreachableActions },
        { tokens::PluginId, lua_tokens::PluginId },
        { tokens::ManualDeviceAdding, lua_tokens::ManualDeviceAdding }
    };
    for( auto& replaceField : replaceTable ) {
        if( auto fieldValue = fields.remove( replaceField.first ) ) {
            fields.insert( replaceField.second, std::move( fieldValue ) );
        }
    }
}

void convertGatewayUserDataFieldNamesFromLuaNames(::common::TableValue& fields ) {
    static std::initializer_list<std::pair<const char*, const char*>> replaceTable = {
        { lua_tokens::UnreachableReasons, tokens::UnreachableReasons },
        { lua_tokens::UnreachableActions, tokens::UnreachableActions },
        { lua_tokens::PluginId, tokens::PluginId },
        { lua_tokens::ManualDeviceAdding, tokens::ManualDeviceAdding }
    };
    for( auto& replaceField : replaceTable ) {
        if( auto fieldValue = fields.remove( replaceField.first ) ) {
            fields.insert( replaceField.second, std::move( fieldValue ) );
        }
    }
}

std::string getPluginNameByItemId( const char* itemId ) {
    // get gatewayId  by itemId
    if( itemId == nullptr ) {
        return {};
    }

    std::vector<char> buff;
    gateways::ItemInfo itemInfo;
    gateways::ObjectSerializer serializer;

    {
        auto items = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );

        if( !( items && items->getBlob( itemId, buff ) && serializer.deserialize( std::move( buff ), itemInfo ) ) ) {
            FUNC_LOGE( "Failed to deserialize itemId: %s", itemId );
            return {};
        }
        else {
            itemInfo.getId();
        }
    }

    auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    gateways::BaseDeviceInfo deviceInfo;
    if( !( devices && devices->getBlob( itemInfo.getDeviceId(), buff ) &&
           serializer.deserialize( itemInfo.getDeviceId(), std::move( buff ), deviceInfo ) ) ) {
        FUNC_LOGE( "Failed to deserialize key: %s", itemInfo.getDeviceId().c_str() );
        return {};
    }

    return { deviceInfo.getGatewayId() };
}

std::string getPluginNameByDeviceId( const char* deviceId ) {
    std::vector<char> buff;
    gateways::ObjectSerializer serializer;

    auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    gateways::BaseDeviceInfo deviceInfo;
    if( !( devices && devices->getBlob( deviceId, buff ) &&
           serializer.deserialize( deviceId, std::move( buff ), deviceInfo ) ) ) {
        FUNC_LOGE( "Failed to deserialize key: %s", deviceId );
        return {};
    }

    return { deviceInfo.getGatewayId() };
}

std::string getPluginNameBySettingId( const char* settingId ) {
    if( settingId == nullptr ) {
        return {};
    }
    if( auto info = ::storage::DeviceSettings::get().getInfo( settingId ) ) {
        return getPluginNameByDeviceId( info->getItemId().c_str() );
    }
    return {};
}

bool fillDevicePath( char* resultBuffer, uint16_t maxBufferSize, char gatewayPrefix, const DeviceId& deviceId ) {
    // Generate path like: /volume/DN7FA.ZD
    // /<VOLUME>/<DEVICE_ID>.<GATEWAY_UNIQUE_SYMBOL>D

    return snprintf( resultBuffer,
                     maxBufferSize,
                     "%s/%" PRIX32 ".%c%c",
                     storage::Volume::m_basePath,
                     deviceId,
                     DeviceFileExtensionSymbol,
                     gatewayPrefix ) > 0;
}

bool fillDevicePath( char* resultBuffer, uint16_t maxBufferSize, char gatewayPrefix, std::string deviceId ) {
    return snprintf( resultBuffer,
                     maxBufferSize,
                     "%s/%s" PRIX32 ".%c%c",
                     storage::Volume::m_basePath,
                     deviceId.c_str(),
                     DeviceFileExtensionSymbol,
                     gatewayPrefix ) > 0;
}

bool fillPublicItemId( char* resultBuffer,
                       uint16_t maxBuffferSize,
                       GatewayID gatewayId,
                       const DeviceId& deviceId,
                       const ItemId& itemIdx ) {
    return snprintf( resultBuffer,
                     maxBuffferSize,
                     "%c%" PRIX32 "-%" PRIX32,
                     toDevicePrefix( gatewayId ),
                     deviceId,
                     itemIdx ) > 0;
}

bool extractDeviceIdFromPublicId( const char* publicDeviceId, GatewayID& gatewayId, DeviceId& deviceId ) {
    assert( publicDeviceId != nullptr );

    auto gateway = getByDevicePrefix( publicDeviceId[0] );
    if( gateway == nullptr ) {
        return false;
    }

    gatewayId = gateway->getTypeId();

    if( 1 != sscanf( publicDeviceId + 1, "%" PRIX32, &deviceId ) ) {
        return ( 1 == sscanf( publicDeviceId + 1, "%" PRIx32, &deviceId ) );
    }

    return true;
}

bool extractItemIdFromPublicId( const char* publicItemId, GatewayID& gatewayId, DeviceId& deviceId, ItemId& itemId ) {
    char gatewayPrefix = '\0';
    if( 3 == sscanf( publicItemId, "%c%" PRIX32 "-%" PRIX32, &gatewayPrefix, &deviceId, &itemId ) ) {
        auto gateway = getByDevicePrefix( gatewayPrefix );
        if( gateway == nullptr ) {
            return false;
        }

        gatewayId = gateway->getTypeId();
        return true;
    }

    return false;
}

bool writeDeviceInfo( common::JsonStreamGenerator& jsonGenerator, const BaseDeviceInfo& deviceInfo ) {
    bool result = { true };
    result = result && jsonGenerator.writeProperty( tokens::SubId, deviceInfo.getId() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceTypeId, deviceInfo.getDeviceTypeId() );
    result = result && jsonGenerator.writeProperty( tokens::ParentDeviceId, deviceInfo.getParentDeviceId() );
    result = result && jsonGenerator.writeProperty( tokens::Category, deviceInfo.getCategory() );
    result = result && jsonGenerator.writeProperty( tokens::SubCategory, deviceInfo.getSubcategory() );
    result = result && jsonGenerator.writeProperty( tokens::GatewayId, deviceInfo.getGatewayId() );
    result = result && jsonGenerator.writeProperty( tokens::Name, deviceInfo.getName() );
    result = result && jsonGenerator.writeProperty( tokens::Type, deviceInfo.getType() );
    result = result && jsonGenerator.writeProperty( tokens::BatteryPowered, deviceInfo.isBatteryPowered() );
    result = result && jsonGenerator.writeProperty( tokens::Reachable, deviceInfo.isReachable() );
    result = result && jsonGenerator.writeProperty( tokens::Persistent, deviceInfo.isPersistent() );
    result = result && jsonGenerator.writeProperty( tokens::ServiceNotification, deviceInfo.isServiceNotification() );
    if( core::HouseModesManager::get().isDeviceModed(
            deviceInfo.getId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) ) {
        result = result && jsonGenerator.writeProperty( tokens::Armed, deviceInfo.isArmed() );
    }
    result = result && jsonGenerator.writeProperty( tokens::RoomId, deviceInfo.getRoomId() );
    result = result && jsonGenerator.writeProperty( tokens::Security, deviceInfo.getSecurity() );
    result = result && jsonGenerator.writeProperty( tokens::Ready, deviceInfo.isReady() );
    result = result && jsonGenerator.writeProperty( tokens::Status, deviceInfo.getStatus() );
    result = result && jsonGenerator.writePropertyKey( tokens::Info );
    result = result && jsonGenerator.writeOpeningObject();

    auto table = deviceInfo.getInfo();

    for( auto it = table.begin(); it != table.end(); it++ ) {
        std::string key = it->first;
        std::string value = it->second;
        result = result && jsonGenerator.writeProperty( key.c_str(), value.c_str() );
    }

    result = result && jsonGenerator.writeClosingSymbol();

    auto firmware_table = deviceInfo.getFirmware();
    if( !firmware_table.empty() ) {
        result = result && jsonGenerator.writePropertyKey( tokens::Firmware );
        result = result && jsonGenerator.writeOpeningArray();

        for( auto it = firmware_table.begin(); it != firmware_table.end(); it++ ) {
            result = result && jsonGenerator.writeOpeningObject();
            std::string id = it->first;
            std::string version = it->second;
            const auto idStr = id.substr( id.find( " " ) + 1 );
            result = result && jsonGenerator.writeProperty( "id", idStr.c_str() );
            result = result && jsonGenerator.writeProperty( "version", version.c_str() );
            result = result && jsonGenerator.writeClosingSymbol();
        }

        result = result && jsonGenerator.writeClosingSymbol();
    }

    return result;
}

bool filteredParam( std::vector<std::string> filter, std::string param ) {
    return filter.end() != find( filter.begin(), filter.end(), param );
};

bool writeDeviceInfoWithFilter( common::JsonStreamGenerator& jsonGenerator,
                                const BaseDeviceInfo& deviceInfo,
                                std::vector<std::string> filter ) {
    bool result = { true };
    if( filteredParam( filter, tokens::DeviceTypeId ) )
        result = result && jsonGenerator.writeProperty( tokens::DeviceTypeId, deviceInfo.getDeviceTypeId() );

    if( filteredParam( filter, tokens::ParentDeviceId ) )
        result = result && jsonGenerator.writeProperty( tokens::ParentDeviceId, deviceInfo.getParentDeviceId() );

    if( filteredParam( filter, tokens::Category ) )
        result = result && jsonGenerator.writeProperty( tokens::Category, deviceInfo.getCategory() );

    if( filteredParam( filter, tokens::SubCategory ) )
        result = result && jsonGenerator.writeProperty( tokens::SubCategory, deviceInfo.getSubcategory() );

    if( filteredParam( filter, tokens::GatewayId ) )
        result = result && jsonGenerator.writeProperty( tokens::GatewayId, deviceInfo.getGatewayId() );

    if( filteredParam( filter, tokens::Name ) )
        result = result && jsonGenerator.writeProperty( tokens::Name, deviceInfo.getName() );

    if( filteredParam( filter, tokens::Type ) )
        result = result && jsonGenerator.writeProperty( tokens::Type, deviceInfo.getType() );

    if( filteredParam( filter, tokens::BatteryPowered ) )
        result = result && jsonGenerator.writeProperty( tokens::BatteryPowered, deviceInfo.isBatteryPowered() );

    if( filteredParam( filter, tokens::Reachable ) )
        result = result && jsonGenerator.writeProperty( tokens::Reachable, deviceInfo.isReachable() );

    if( filteredParam( filter, tokens::Persistent ) )
        result = result && jsonGenerator.writeProperty( tokens::Persistent, deviceInfo.isPersistent() );

    if( filteredParam( filter, tokens::ServiceNotification ) )
        result =
            result && jsonGenerator.writeProperty( tokens::ServiceNotification, deviceInfo.isServiceNotification() );

    if( filteredParam( filter, tokens::Armed ) ) {
        if( core::HouseModesManager::get().isDeviceModed(
                deviceInfo.getId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) ) {
            result = result && jsonGenerator.writeProperty( tokens::Armed, deviceInfo.isArmed() );
        }
        else {
            result = result && jsonGenerator.writeProperty( tokens::Armed, false );
        }
    }

    if( filteredParam( filter, tokens::RoomId ) )
        result = result && jsonGenerator.writeProperty( tokens::RoomId, deviceInfo.getRoomId() );

    if( filteredParam( filter, tokens::Security ) )
        result = result && jsonGenerator.writeProperty( tokens::Security, deviceInfo.getSecurity() );

    if( filteredParam( filter, tokens::Ready ) )
        result = result && jsonGenerator.writeProperty( tokens::Ready, deviceInfo.isReady() );

    if( filteredParam( filter, tokens::Status ) )
        result = result && jsonGenerator.writeProperty( tokens::Status, deviceInfo.getStatus() );

    if( filteredParam( filter, tokens::Info ) ) {
        result = result && jsonGenerator.writePropertyKey( tokens::Info );
        result = result && jsonGenerator.writeOpeningObject();

        auto table = deviceInfo.getInfo();

        for( auto it = table.begin(); it != table.end(); it++ ) {
            std::string key = it->first;
            std::string value = it->second;
            result = result && jsonGenerator.writeProperty( key.c_str(), value.c_str() );
        }

        result = result && jsonGenerator.writeClosingSymbol();
    }

    return result;
}

bool writeItemInfo( common::JsonStreamGenerator& jsonGenerator, const ItemInfo& itemInfo ) {
    using namespace common::resources;

    BaseDeviceInfo deviceInfo;
    if( itemInfo.getDeviceId().empty() ) {
        return false;
    }
    else {
        auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
        if( !devices ) {
            return false;
        }
        std::vector<char> buff;
        if( !devices->getBlob( itemInfo.getDeviceId(), buff ) ) {
            return false;
        }
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( itemInfo.getDeviceId(), std::move( buff ), deviceInfo ) ) {
            return false;
        }
    }
    bool result = { true };

    result = result && jsonGenerator.writeProperty( tokens::SubId, itemInfo.getId() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceId, itemInfo.getDeviceId() );
    if( core::HouseModesManager::get().isDeviceModed(
            itemInfo.getDeviceId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) ) {
        /// TODO: add filter by item name
        result = result && jsonGenerator.writeProperty( tokens::DeviceName, deviceInfo.getName() );
        result = result && jsonGenerator.writeProperty(
                               tokens::DeviceArmed,
                               core::HouseModesManager::get().isDeviceArmed(
                                   itemInfo.getDeviceId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) );
    }
    result = result && jsonGenerator.writeProperty( tokens::HasGetter, itemInfo.hasGetter() );
    result = result && jsonGenerator.writeProperty( tokens::HasSetter, itemInfo.hasSetter() );
    result = result && jsonGenerator.writeProperty( tokens::Name, itemInfo.getName() );
    result = result && jsonGenerator.writeProperty( tokens::Show, itemInfo.isVisible() );

    for( auto& field : itemInfo.getTypeSpecificFields() ) {
        result = result && jsonGenerator.writeProperty( field.first.c_str(), field.second.get() );
    }

    auto itemValue = itemInfo.getValue();
    if( result && itemValue ) {
        result = writeItemValue( jsonGenerator, itemInfo, *itemValue );
    }

    if( result && !itemInfo.getUserCodeRestriction().empty() ) {
        result = jsonGenerator.writeProperty( common::resources::tokens::UserCodeRestriction,
                                              itemInfo.getUserCodeRestriction() );
        if( result && itemInfo.getElementsMaxNumber() != 0 ) {
            result = jsonGenerator.writeProperty( common::resources::tokens::ElementsMaxNumber,
                                                  itemInfo.getElementsMaxNumber() );
        }
    }

    return result;
}

bool writeItemInfoWithFilter( common::JsonStreamGenerator& jsonGenerator,
                              const ItemInfo& itemInfo,
                              std::vector<std::string> filter ) {
    using namespace common::resources;

    auto filteredParam = [filter]( std::string param ) -> bool {
        return filter.end() != find( filter.begin(), filter.end(), param );
    };

    BaseDeviceInfo deviceInfo;
    if( itemInfo.getDeviceId().empty() ) {
        return false;
    }
    else {
        auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
        if( !devices ) {
            return false;
        }
        std::vector<char> buff;
        if( !devices->getBlob( itemInfo.getDeviceId(), buff ) ) {
            return false;
        }
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( itemInfo.getDeviceId(), std::move( buff ), deviceInfo ) ) {
            return false;
        }
    }

    bool result = { true };

    if( filteredParam( tokens::DeviceId ) ) {
        result = result && jsonGenerator.writeProperty( tokens::DeviceId, itemInfo.getDeviceId() );
    }

    if( filteredParam( tokens::HasGetter ) ) {
        result = result && jsonGenerator.writeProperty( tokens::HasGetter, itemInfo.hasGetter() );
    }

    if( filteredParam( tokens::HasSetter ) ) {
        result = result && jsonGenerator.writeProperty( tokens::HasSetter, itemInfo.hasSetter() );
    }

    if( filteredParam( tokens::Name ) ) {
        result = result && jsonGenerator.writeProperty( tokens::Name, itemInfo.getName() );
    }

    if( filteredParam( tokens::Show ) ) {
        result = result && jsonGenerator.writeProperty( tokens::Show, itemInfo.isVisible() );
    }

    if( filteredParam( tokens::Value ) ) {
        auto itemValue = itemInfo.getValue();
        if( result && itemValue ) {
            result = result && jsonGenerator.writeProperty( tokens::Value, *itemValue );
        }
    }

    if( filteredParam( tokens::Status ) ) {
        result = result && jsonGenerator.writeProperty( tokens::Status, itemInfo.getStatus() );
    }

    if( filteredParam( tokens::Scale ) ) {
        if( auto itemValue = itemInfo.getValue() ) {
            auto scaleble = common::iValueCast<common::ScalebleValue>( std::move( itemValue ) );
            if( scaleble ) {
                result = result && jsonGenerator.writeProperty( tokens::Scale, scaleble->getScale() );
            }
        }
    }

    if( filteredParam( tokens::ValueType ) ) {
        auto itemValue = itemInfo.getValue();
        if( result && itemValue ) {
            result = jsonGenerator.writeProperty( tokens::ValueType, toString( itemInfo.getValueType() ) );
        }
    }

    if( filteredParam( tokens::Enum ) ) {
        result = jsonGenerator.writePropertyKey( tokens::Enum ) && jsonGenerator.writeOpeningArray();
        const auto enumsCount = itemInfo.getEnumItemsCount();
        for( auto idx = 0; idx < enumsCount && result; ++idx ) {
            auto enumStr = itemInfo.getEnumValue( idx );
            result = jsonGenerator.writeProperty( nullptr, enumStr );
        }

        if( !jsonGenerator.writeClosingSymbol() ) {
            result = false;
        }
    }

    if( filteredParam( tokens::MinValue ) ) {
        auto minmax = itemInfo.getMinMax();
        result = jsonGenerator.writeProperty( tokens::MinValue, minmax.first );
    }

    if( filteredParam( tokens::MaxValue ) ) {
        auto minmax = itemInfo.getMinMax();
        result = result && jsonGenerator.writeProperty( tokens::MaxValue, minmax.second );
    }

    if( filteredParam( common::resources::tokens::UserCodeRestriction ) ) {
        if( result && !itemInfo.getUserCodeRestriction().empty() ) {
            result = jsonGenerator.writeProperty( common::resources::tokens::UserCodeRestriction,
                                                  itemInfo.getUserCodeRestriction() );
        }
    }

    if( filteredParam( tokens::ElementsMaxNumber ) ) {
        if( result && itemInfo.getElementsMaxNumber() != 0 ) {
            result = jsonGenerator.writeProperty( tokens::ElementsMaxNumber, itemInfo.getElementsMaxNumber() );
        }
    }

    if( filteredParam( tokens::ElementsMaxNumberPerArray ) ) {
        auto& fields = itemInfo.getTypeSpecificFields();
        auto iterElementsMaxNumberPerArray = fields.find( tokens::ElementsMaxNumberPerArray );
        if( iterElementsMaxNumberPerArray != fields.end() ) {
            if( auto elementsMaxNumberPerArray =
                    ::common::iValueCast<::common::IntValue>( iterElementsMaxNumberPerArray->second.get() ) ) {
                result =
                    jsonGenerator.writeProperty( tokens::ElementsMaxNumberPerArray, elementsMaxNumberPerArray->get() );
            }
            else {
                FUNC_LOGW( "%s cannot cast to int", tokens::ElementsMaxNumberPerArray );
            }
        }
    }

    if( filteredParam( tokens::OneWeekDayCost ) ) {
        auto& fields = itemInfo.getTypeSpecificFields();
        auto iterOneWeekDayCost = fields.find( tokens::OneWeekDayCost );
        auto iterOneShiftedWeekDayCost = fields.find( tokens::OneShiftedWeekDayCost );
        if( iterOneWeekDayCost != fields.end() && iterOneShiftedWeekDayCost != fields.end() ) {
            auto oneWeekDayCost = ::common::iValueCast<::common::IntValue>( iterOneWeekDayCost->second.get() );
            result = jsonGenerator.writeProperty( tokens::ElementsMaxNumberPerArray, oneWeekDayCost->get() );
        }
    }

    if( filteredParam( tokens::OneShiftedWeekDayCost ) ) {
        auto& fields = itemInfo.getTypeSpecificFields();
        auto iterOneWeekDayCost = fields.find( tokens::OneWeekDayCost );
        auto iterOneShiftedWeekDayCost = fields.find( tokens::OneShiftedWeekDayCost );
        if( iterOneWeekDayCost != fields.end() && iterOneShiftedWeekDayCost != fields.end() ) {
            auto oneShiftedWeekDayCost =
                ::common::iValueCast<::common::IntValue>( iterOneShiftedWeekDayCost->second.get() );
            result = jsonGenerator.writeProperty( tokens::ElementsMaxNumberPerArray, oneShiftedWeekDayCost->get() );
        }
    }

    return result;
}

bool writeSettingInfo( common::JsonStreamGenerator& jsonGenerator, const DeviceSettingsInfo& settingInfo ) {
    using namespace common::resources;

    bool result = { true };

    result = result && jsonGenerator.writeProperty( tokens::SubId, settingInfo.getId() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceId, settingInfo.getItemId() );

    auto label = settingInfo.getLabel();
    if( !( label.lang_tag.empty() && label.text.empty() ) ) {
        result = jsonGenerator.writePropertyKey( tokens::Label ) && jsonGenerator.writeOpeningObject();
        if( !label.lang_tag.empty() ) {
            result = result && jsonGenerator.writeProperty( tokens::LangTag, label.lang_tag );
        }
        if( !label.text.empty() ) {
            result = result && jsonGenerator.writeProperty( tokens::Text, label.text );
        }
        result = jsonGenerator.writeClosingSymbol();
    }

    if( auto status = ::storage::DeviceSettings::get().getStatus( settingInfo.getId() ) ) {
        result = result && jsonGenerator.writeProperty( tokens::Status, *status );
    }
    if( auto settingValue = settingInfo.getValue() ) {
        result = result && writeSettingValue( jsonGenerator, settingInfo, *settingValue );
    }

    if( settingInfo.hasZwaveParameters() ) {
        result = jsonGenerator.writePropertyKey( tokens::Info ) && jsonGenerator.writeOpeningObject();
        result = result &&
                 jsonGenerator.writeProperty( tokens::ZwaveParameterNumber, settingInfo.getZwaveParameterNumber() );
        result =
            result && jsonGenerator.writeProperty( tokens::ZwaveParameterSize, settingInfo.getZwaveParameterSize() );
        result = jsonGenerator.writeClosingSymbol();
    }

    if( settingInfo.hasDescriptor() ) {
        result = result && jsonGenerator.writeProperty( tokens::Descriptor, *settingInfo.getDescriptor() );
    }

    return result;
}

bool writeGatewaySettingsInfo( common::JsonStreamGenerator& jsonGenerator, const GatewaySettingsInfo& settingInfo ) {
    using namespace common::resources;

    bool result = { true };

    result = result && jsonGenerator.writeProperty( tokens::SubId, settingInfo.getId() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceId, settingInfo.getItemId() );

    auto label = settingInfo.getLabel();
    if( !( label.lang_tag.empty() && label.text.empty() ) ) {
        result = jsonGenerator.writePropertyKey( tokens::Label ) && jsonGenerator.writeOpeningObject();
        if( !label.lang_tag.empty() ) {
            result = result && jsonGenerator.writeProperty( tokens::LangTag, label.lang_tag );
        }
        if( !label.text.empty() ) {
            result = result && jsonGenerator.writeProperty( tokens::Text, label.text );
        }
        result = jsonGenerator.writeClosingSymbol();
    }

    auto& storage = ::storage::GatewaySettings::get();
    if( auto status = storage.getStatus( settingInfo.getId() ) ) {
        result = result && jsonGenerator.writeProperty( tokens::Status, *status );
    }
    if( auto settingValue = settingInfo.getValue() ) {
        result = result && writeGatewaySettingValue( jsonGenerator, settingInfo, *settingValue );
    }

    return result;
}

bool writeUpdatedField( common::JsonStreamGenerator& jsonGenerator,
                        const char* id,
                        const char* fieldId,
                        const common::IValue* fieldValue ) {
    using namespace common::resources;

    bool result = { true };
    if( id == nullptr || fieldId == nullptr || fieldValue == nullptr ) {
        return false;
    }

    result = result && jsonGenerator.writeProperty( tokens::SubId, id );
    if( result && fieldValue ) {
        result = result && jsonGenerator.writeProperty( fieldId, *fieldValue );
    }

    return result;
}

bool writeItemUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                           const ItemInfo& itemInfo,
                           common::IValue* value ) {
    using namespace common::resources;

    gateways::BaseDeviceInfo deviceInfo;
    if( itemInfo.getDeviceId().empty() ) {
        return false;
    }
    else {
        auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
        if( !devices ) {
            return false;
        }
        std::vector<char> buff;
        if( !devices->getBlob( itemInfo.getDeviceId(), buff ) ) {
            return false;
        }
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( itemInfo.getDeviceId(), std::move( buff ), deviceInfo ) ) {
            return false;
        }
    }

    bool result = true;

    result = result && jsonGenerator.writeProperty( tokens::SubId, itemInfo.getId() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceId, itemInfo.getDeviceId() );
    /// TODO: add filter by item name
    result = result && jsonGenerator.writeProperty( tokens::DeviceName, deviceInfo.getName() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceCategory, deviceInfo.getCategory() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceSubCategory, deviceInfo.getSubcategory() );
    result = result && jsonGenerator.writeProperty( tokens::ServiceNotification, deviceInfo.isServiceNotification() );
    {
        auto pagesManager = core::PagesManager::get();
        if( pagesManager ) {
            auto roomNameBuffer = pagesManager->getPageName( deviceInfo.getRoomId() );
            result = result && jsonGenerator.writeProperty( tokens::RoomName, roomNameBuffer );
        }
        else {
            result = false;
        }
    }
    result = result &&
             jsonGenerator.writeProperty( tokens::UserNotification,
                                          core::HouseModesManager::get().isNotificationSendToUser( itemInfo, *value ) );
    result = result && jsonGenerator.writePropertyKey( common::resources::tokens::Notifications );
    result = result && ( core::HouseModesManager::Result::OK ==
                         core::HouseModesManager::get().getCurrentNotifications( jsonGenerator ) );
    result = result && jsonGenerator.writeProperty( tokens::Name, itemInfo.getName() );
    for( auto& field : itemInfo.getTypeSpecificFields() ) {
        result = result && jsonGenerator.writeProperty( field.first.c_str(), field.second.get() );
    }
    if( core::HouseModesManager::get().isDeviceModed(
            itemInfo.getDeviceId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) ) {
        result = result && jsonGenerator.writeProperty(
                               tokens::DeviceArmed,
                               core::HouseModesManager::get().isDeviceArmed(
                                   itemInfo.getDeviceId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) );
    }

    if( result && value ) {
        result = writeItemValue( jsonGenerator, itemInfo, *value );
    }

    return result;
}

bool writeItemDictionaryUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                                     const ItemInfo& itemInfo,
                                     const std::string operation,
                                     const std::string itemDictionaryId,
                                     common::IValue* value ) {
    using namespace common::resources;

    gateways::BaseDeviceInfo deviceInfo;
    if( itemInfo.getDeviceId().empty() ) {
        return false;
    }
    else {
        auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
        if( !devices ) {
            return false;
        }
        std::vector<char> buff;
        if( !devices->getBlob( itemInfo.getDeviceId(), buff ) ) {
            return false;
        }
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( itemInfo.getDeviceId(), std::move( buff ), deviceInfo ) ) {
            return false;
        }
    }

    bool result = true;

    result = result && jsonGenerator.writeProperty( tokens::SubId, itemInfo.getId() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceId, itemInfo.getDeviceId() );
    /// TODO: add filter by item name
    result = result && jsonGenerator.writeProperty( tokens::DeviceName, deviceInfo.getName() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceCategory, deviceInfo.getCategory() );
    result = result && jsonGenerator.writeProperty( tokens::DeviceSubCategory, deviceInfo.getSubcategory() );
    result = result && jsonGenerator.writeProperty( tokens::ServiceNotification, deviceInfo.isServiceNotification() );
    {
        auto pagesManager = core::PagesManager::get();
        if( pagesManager ) {
            auto roomNameBuffer = pagesManager->getPageName( deviceInfo.getRoomId() );
            result = result && jsonGenerator.writeProperty( tokens::RoomName, roomNameBuffer );
        }
        else {
            result = false;
        }
    }
    result = result &&
             jsonGenerator.writeProperty( tokens::UserNotification,
                                          core::HouseModesManager::get().isNotificationSendToUser( itemInfo, *value ) );

    auto parentDeviceName = core::HouseModesManager::get().parentDeviceName( itemInfo, *value );

    if( !parentDeviceName.empty() ) {
        result = result && jsonGenerator.writeProperty( tokens::ParentDeviceName, parentDeviceName );
    }

    result = result && jsonGenerator.writePropertyKey( common::resources::tokens::Notifications );
    result = result && ( core::HouseModesManager::Result::OK ==
                         core::HouseModesManager::get().getCurrentNotifications( jsonGenerator ) );
    result = result && jsonGenerator.writeProperty( tokens::Name, itemInfo.getName() );
    for( auto& field : itemInfo.getTypeSpecificFields() ) {
        result = result && jsonGenerator.writeProperty( field.first.c_str(), field.second.get() );
    }
    if( core::HouseModesManager::get().isDeviceModed(
            itemInfo.getDeviceId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) ) {
        result = result && jsonGenerator.writeProperty(
                               tokens::DeviceArmed,
                               core::HouseModesManager::get().isDeviceArmed(
                                   itemInfo.getDeviceId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) );
    }

    result = result && jsonGenerator.writeProperty( tokens::Operation, operation );
    result = result && jsonGenerator.writePropertyKey( common::resources::tokens::Element );
    result = result && jsonGenerator.writeOpeningObject();
    result = result && jsonGenerator.writePropertyKey( itemDictionaryId.c_str() );
    result = result && jsonGenerator.writeOpeningObject();

    if( result && value ) {
        result = writeItemValue( jsonGenerator, itemInfo, *value );
    }

    result = result && jsonGenerator.writeProperty( tokens::SyncNotification, false );

    result = result && jsonGenerator.writeClosingSymbol();
    result = result && jsonGenerator.writeClosingSymbol();

    return result;
}

bool writeSettingUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                              const SettingsInfo& settingInfo,
                              common::IValue* value ) {
    using namespace common::resources;

    bool result = true;

    result = result && jsonGenerator.writeProperty( tokens::SubId, settingInfo.getId() );
    if( result && value ) {
        result = writeSettingValue( jsonGenerator, settingInfo, *value );
    }
    return result;
}

bool writeGatewaySettingsUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                                      const GatewaySettingsInfo& settingInfo,
                                      const common::IValue* value ) {
    using namespace common::resources;

    bool result = true;

    result = result && jsonGenerator.writeProperty( tokens::SubId, settingInfo.getId() );
    if( result && value ) {
        result = writeGatewaySettingValue( jsonGenerator, settingInfo, *value );
    }

    return result;
}

bool writeSettingStatusUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                                    const std::string& settingId,
                                    const std::string& status ) {
    using namespace common::resources;

    bool result = true;

    result = result && jsonGenerator.writeProperty( tokens::SubId, settingId );
    if( result ) {
        result = jsonGenerator.writeProperty( tokens::Status, status );
    }

    return result;
}

bool writeSettingDictionaryUpdatedInfo( common::JsonStreamGenerator& jsonGenerator,
                                        const DeviceSettingsInfo& settingInfo,
                                        const std::string operation,
                                        const std::string key,
                                        const common::IValue* value ) {
    using namespace common::resources;

    bool result = true;

    result = result && jsonGenerator.writeProperty( tokens::SubId, settingInfo.getId() );
    result = result && jsonGenerator.writeProperty( tokens::Operation, operation );
    result = result && jsonGenerator.writePropertyKey( common::resources::tokens::Element );
    result = result && jsonGenerator.writeOpeningObject();
    result = result && jsonGenerator.writePropertyKey( key.c_str() );
    result = result && jsonGenerator.writeOpeningObject();
    if( value ) {
        result = result && jsonGenerator.writeProperty( tokens::Value, *value );
    }
    result = result && jsonGenerator.writeClosingSymbol();
    result = result && jsonGenerator.writeClosingSymbol();

    result = result && jsonGenerator.writeProperty( tokens::SyncNotification, false );

    return result;
}

const char* addingModeToString( znet::ProvisioningListEntryFields::AddingMode addingMode ) {
    switch( addingMode ) {
    case znet::ProvisioningListEntryFields::AddingMode::S2:
        return "s2";
    case znet::ProvisioningListEntryFields::AddingMode::SMART:
        return "smart";
    default:
        return "undefined";
    }
}

bool writeProvisioningListEntryFields(::common::JsonStreamGenerator& gen,
                                      const ::znet::ProvisioningListEntryFields& fields ) {
    bool success = true;
    if( fields.m_node ) {
        success = success && gen.writeProperty( provisioning::NodeId, *fields.m_node );
    }
    if( fields.m_status ) {
        switch( *fields.m_status ) {
            using Status = ::znet::ProvisioningListEntryFields::Status;
        case Status::NOT_DETECTED:
            success = success && gen.writeProperty( tokens::Status, "notDetected" );
            break;
        case Status::ADDED:
            success = success && gen.writeProperty( tokens::Status, "added" );
            break;
        case Status::OTHER_NETWORK:
            success = success && gen.writeProperty( tokens::Status, "otherNetwork" );
            break;
        case Status::FAILED:
            success = success && gen.writeProperty( tokens::Status, "failed" );
            break;
        };
    }
    if( fields.m_addingMode ) {
        success = success && gen.writeProperty( provisioning::AddingMode, addingModeToString( *fields.m_addingMode ) );
    }
    if( fields.m_supportedAddingModes.size() > 0 ) {
        success = success && gen.writePropertyKey( provisioning::SupportedAddingModes );
        success = success && gen.writeOpeningArray();
        for( auto& it : fields.m_supportedAddingModes ) {
            success = success && gen.writeProperty( nullptr, addingModeToString( it ) );
            if( !success ) {
                break;
            }
        }
        success = success && gen.writeClosingSymbol();
    }
    if( fields.m_product ) {
        success = success && gen.writePropertyKey( provisioning::Product );
        success = success && gen.writeOpeningObject();
        success = success && gen.writeProperty( tokens::Manufacturer, fields.m_product->m_manufacturerId );
        success = success && gen.writeProperty( tokens::Type, fields.m_product->m_productType );
        success = success && gen.writeProperty( tokens::Id, fields.m_product->m_productId );
        success = success && gen.writeProperty( tokens::Version, fields.m_product->m_version );
        success = success && gen.writeProperty( tokens::SubVersion, fields.m_product->m_subversion );
        success = success && gen.writeClosingSymbol();
    }
    if( fields.m_type ) {
        success = success && gen.writePropertyKey( tokens::Type );
        success = success && gen.writeOpeningObject();
        success = success && gen.writeProperty( provisioning::Generic, fields.m_type->m_generic );
        success = success && gen.writeProperty( provisioning::Specific, fields.m_type->m_specific );
        success = success && gen.writeProperty( provisioning::Icon, fields.m_type->m_icon );
        success = success && gen.writeClosingSymbol();
    }
    if( fields.m_maxRequestInterval ) {
        success = success && gen.writeProperty( provisioning::MaxRequestInterval, *fields.m_maxRequestInterval );
    }
    if( fields.m_uuid ) {
        success = success && gen.writeProperty( tokens::Uuid, *fields.m_uuid );
    }
    if( fields.m_name ) {
        success = success && gen.writeProperty( tokens::Name, *fields.m_name );
    }
    if( fields.m_room ) {
        success = success && gen.writeProperty( provisioning::Room, *fields.m_room );
    }
    if( fields.m_securityModes ) {
        success =
            success && gen.writeProperty( provisioning::SecurityModes,
                                          ::znet::security_modes_converter::bitsetToList( *fields.m_securityModes ) );
    }
    if( fields.m_nodeSecurityModes ) {
        success = success &&
                  gen.writeProperty( provisioning::NodeSecurityModes,
                                     ::znet::security_modes_converter::bitsetToList( *fields.m_nodeSecurityModes ) );
    }
    if( fields.m_supportedModes ) {
        success =
            success && gen.writeProperty( provisioning::SupportedModes,
                                          ::znet::security_modes_converter::bitsetToList( *fields.m_supportedModes ) );
    }
    return success;
}

}  // namespace utils
}  // namespace gateways
