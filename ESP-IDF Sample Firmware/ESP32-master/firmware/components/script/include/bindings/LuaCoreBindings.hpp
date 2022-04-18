#pragma once

#include "settings/responces/SettingUpdated.h"
#include "LuaBinding.hpp"
#include "Lua.h"

#include "values/TableValue.h"
#include "values/ArrayValue.h"
#include "values/IntValue.h"
#include "values/StringValue.h"
#include "converters/ScalesConverterVisitor.hpp"
#include "IValueCast.h"

#include "SettingStorage.h"
#include "IGatewayUserDataStorage.h"
#include "CommonResources.h"
#include "LuaModuleCore.h"
#include "DebugUtils.h"
#include "DeviceInfo.h"
#include "HubSettings.h"
#include "HubSettingsInfo.h"

#include "SettingsInfo.h"
#include "DeviceSettingsInfo.hpp"
#include "GatewaySettingsInfo.hpp"

#include "gregorian_calendar.h"

#include "settings/responces/SettingAdded.h"
#include "settings/responces/SettingRemoved.h"
#include "settings/responces/SettingValueSetted.h"
#include "settings/responces/SettingDictionaryValueUpdated.h"
#include "settings/responces/SettingDictionaryStatusUpdated.hpp"
#include "settings/responces/SettingStatusUpdated.h"

#include "settings/responces/GatewaySettingAdded.h"
#include "settings/responces/GatewaySettingRemoved.h"
#include "settings/responces/GatewaySettingValueSetted.h"
#include "settings/responces/GatewaySettingStatusUpdated.h"

static constexpr auto NMA_NAME = "nma";

namespace lua {
namespace module {
namespace bindings {
namespace core {

class AddSetting : public LuaBinding<std::string( std::unique_ptr<common::TableValue> )> {
public:
    static auto parseValueType(::common::TableValue& setting ) {
        namespace tokens = ::common::resources::lua_events;
        using ValueType = ::gateways::ItemValueType;
        using OptionalValueType = std::optional<ValueType>;
        auto valueTypeStr = ::common::iValueCast<::common::StringValue>( setting[tokens::ValueType] );
        if( !valueTypeStr ) {
            return std::make_pair( ValueType::NoValue, OptionalValueType{} );
        }
        else if( valueTypeStr->get() == tokens::DictionaryZwaveConfiguration ) {
            return std::make_pair( ValueType::Dictionary, OptionalValueType{ ValueType::Zwave_configuration } );
        }
        else if( valueTypeStr->get() == tokens::ArrayToken ) {
            return std::make_pair( ValueType::Array, OptionalValueType{ ValueType::Token } );
        }
        else if( auto elementTypeStr = ::common::iValueCast<::common::StringValue>( setting[tokens::ElementType] ) ) {
            return std::make_pair(::gateways::toItemValueType( valueTypeStr->get() ),
                                  OptionalValueType{ ::gateways::toItemValueType( elementTypeStr->get() ) } );
        }
        else {
            return std::make_pair(::gateways::toItemValueType( valueTypeStr->get() ), OptionalValueType{} );
        }
    }

    std::string operator()( std::unique_ptr<::common::TableValue> setting ) const {
        if( !setting ) {
            return std::string();
        }

        std::unique_ptr<gateways::DeviceSettingsInfo> settingInfo;
        char settingId[gateways::DeviceSettingsManager::MaxNameSize];
        std::string name = "setting_";
        auto writtenBytes =
            snprintf( settingId, sizeof( settingId ), "%s%X", name.c_str(), hard::common::generateUniqueId() );
        if( writtenBytes <= 0 || writtenBytes == gateways::DeviceSettingsManager::MaxNameSize ) {
            return std::string();
        }

        std::string device_id =
            common::iValueCast<common::StringValue>( ( *setting )[common::resources::lua_events::DeviceId] )->get();

        const auto [valueType, elementType] = parseValueType( *setting );

        gateways::SettingsInfo::LabelType label;
        if( auto infoTable =
                common::iValueCast<common::TableValue>( ( *setting )[common::resources::tokens::Label] ) ) {
            if( ( *infoTable ).isKeyExist( "lang_tag" ) ) {
                label.lang_tag = common::iValueCast<common::StringValue>( ( *infoTable )["lang_tag"] )->get();
            }
            if( ( *infoTable ).isKeyExist( "text" ) ) {
                label.text = common::iValueCast<common::StringValue>( ( *infoTable )["text"] )->get();
            }
        }
        if( device_id.empty() || valueType == ::gateways::ItemValueType::NoValue || label.text.empty() ) {
            return std::string();
        }

        settingInfo =
            std::make_unique<gateways::DeviceSettingsInfo>( settingId, device_id, label, valueType, elementType );

        if( auto descriptionTable =
                common::iValueCast<common::TableValue>( ( *setting )[common::resources::tokens::Description] ) ) {
            std::string lang_tag;
            std::string text;
            if( ( *descriptionTable ).isKeyExist( "lang_tag" ) ) {
                lang_tag = common::iValueCast<common::StringValue>( ( *descriptionTable )["lang_tag"] )->get();
            }
            if( ( *descriptionTable ).isKeyExist( "text" ) ) {
                text = common::iValueCast<common::StringValue>( ( *descriptionTable )["text"] )->get();
            }
            settingInfo->setDescription( lang_tag, text );
        }

        if( settingInfo->getValueType() == gateways::ItemValueType::Token ||
            settingInfo->getElementType() == gateways::ItemValueType::Token ) {
            auto enumTableValue =
                common::iValueCast<common::TableValue>( ( *setting )[common::resources::tokens::Enum] );
            if( enumTableValue ) {
                settingInfo->setEnumValues( enumTableValue );
            }
            else {
                return std::string();
            }
        }
        else if( settingInfo->getValueType() == gateways::ItemValueType::Int || settingInfo->isScalable() ) {
            if( ( *setting ).isKeyExist( common::resources::lua_events::ValueMin ) &&
                ( *setting ).isKeyExist( common::resources::lua_events::ValueMax ) ) {
                auto minTable =
                    common::iValueCast<common::TableValue>( ( *setting )[common::resources::lua_events::ValueMin] );
                auto maxTable =
                    common::iValueCast<common::TableValue>( ( *setting )[common::resources::lua_events::ValueMax] );
                bool isNeedUpdateMinMax = false;
                float valMin, valMax;

                if( minTable && minTable->isKeyExist( common::resources::lua_events::Value ) ) {
                    auto newVal = settingInfo->castToItemType( std::move( minTable->clone() ) );
                    if( newVal ) {
                        common::ScalesConverterVisitor visitor(::core::HubSettingsInfo::temperatureScaleToStr(
                                                                   ::core::HubSettings::get().getTemperatureScale() ),
                                                               common::ScalesConverterVisitor::Precision );
                        newVal->accept( visitor );
                        auto ivalMin = common::iValueCast<common::FloatValue>( std::move( newVal ) );
                        if( ivalMin ) {
                            valMin = ivalMin->get();
                            isNeedUpdateMinMax = true;
                        }
                    }
                }

                if( maxTable && maxTable->isKeyExist( common::resources::lua_events::Value ) ) {
                    auto newVal = settingInfo->castToItemType( std::move( maxTable->clone() ) );
                    if( newVal ) {
                        common::ScalesConverterVisitor visitor(::core::HubSettingsInfo::temperatureScaleToStr(
                                                                   ::core::HubSettings::get().getTemperatureScale() ),
                                                               common::ScalesConverterVisitor::Precision );
                        newVal->accept( visitor );
                        auto ivalMax = common::iValueCast<common::FloatValue>( std::move( newVal ) );
                        if( ivalMax ) {
                            valMax = ivalMax->get();
                            isNeedUpdateMinMax = true;
                        }
                    }
                }

                if( isNeedUpdateMinMax ) {
                    settingInfo->setMinMax( { valMin, valMax } );
                }
            }
        }

        if( auto info = common::iValueCast<common::TableValue>( ( *setting )[common::resources::lua_events::Info] ) ) {
            if( auto zwaveParameterNumber = common::iValueCast<common::IntValue>(
                    ( *info )[common::resources::lua_events::ZwaveParameterNumber] ) ) {
                settingInfo->setZwaveParameterNumber( zwaveParameterNumber->get() );
            }
            if( auto zwaveParameterSize = common::iValueCast<common::IntValue>(
                    ( *info )[common::resources::lua_events::ZwaveParameterSize] ) ) {
                settingInfo->setZwaveParameterSize( zwaveParameterSize->get() );
            }
        }

        if( auto descriptor = ( *setting )[common::resources::lua_events::Descriptor] ) {
            settingInfo->setDescriptor( descriptor->clone() );
        }

        auto value = ( *setting ).remove( "value" );
        if( settingInfo->getValueType() == gateways::ItemValueType::Array ) {
            value = settingInfo->castToElementType( std::move( value ) );
        }
        else {
            value = settingInfo->castToItemType( std::move( value ) );
        }

        if( value ) {
            auto scValue = ::common::iValueCast<::common::ScalebleValue>( value->clone() );
            if( scValue ) {
                settingInfo->setRelative( scValue->getRelative() );
                value = std::move( scValue );
            }
        }

        auto& storage = ::storage::DeviceSettings::get();
        if( !storage.setInfo( *settingInfo, ::common::resources::lua_events::Synced, std::move( value ) ) ) {
            return std::string();
        }

        auto bus_command = std::make_unique<settings::responces::SettingAdded>();
        bus_command->m_settingInfo = std::move( settingInfo );
        m_ctx->getExecutor().send( NMA_NAME, std::move( bus_command ) );

        return settingId;
    }
};

class ModifySetting : public LuaBinding<void( std::string, std::unique_ptr<common::TableValue> )> {
public:
    static auto parseValueType(::common::TableValue& setting ) {
        namespace tokens = ::common::resources::lua_events;
        using ValueType = ::gateways::ItemValueType;
        using OptionalValueType = std::optional<ValueType>;
        auto valueTypeStr = ::common::iValueCast<::common::StringValue>( setting[tokens::ValueType] );
        if( !valueTypeStr ) {
            return std::make_pair( ValueType::NoValue, OptionalValueType{} );
        }
        else if( valueTypeStr->get() == tokens::DictionaryZwaveConfiguration ) {
            return std::make_pair( ValueType::Dictionary, OptionalValueType{ ValueType::Zwave_configuration } );
        }
        else if( auto elementTypeStr = ::common::iValueCast<::common::StringValue>( setting[tokens::ElementType] ) ) {
            return std::make_pair(::gateways::toItemValueType( valueTypeStr->get() ),
                                  OptionalValueType{ ::gateways::toItemValueType( elementTypeStr->get() ) } );
        }
        else {
            return std::make_pair(::gateways::toItemValueType( valueTypeStr->get() ), OptionalValueType{} );
        }
    }

    void operator()( std::string settingId, std::unique_ptr<::common::TableValue> setting ) const {
        if( settingId.empty() || !setting ) {
            return;
        }

        auto settingInfo = ::storage::DeviceSettings::get().getInfo( settingId );
        if( !settingInfo ) {
            FUNC_LOGE( "Could not get device setting info id=%s", settingId.c_str() );
        }

        auto bus_command = std::make_unique<settings::responces::SettingUpdated>();
        {
            // Label
            gateways::SettingsInfo::LabelType label;
            const char* key = common::resources::tokens::Label;
            if( auto infoTable = common::iValueCast<common::TableValue>( ( *setting )[key] ) ) {
                if( ( *infoTable ).isKeyExist( "lang_tag" ) ) {
                    label.lang_tag = common::iValueCast<common::StringValue>( ( *infoTable )["lang_tag"] )->get();
                }
                if( ( *infoTable ).isKeyExist( "text" ) ) {
                    label.text = common::iValueCast<common::StringValue>( ( *infoTable )["text"] )->get();
                }
                settingInfo->setLabel( label );
            }

            // ValueType
            const auto [valueType, elementType] = parseValueType( *setting );
            if( valueType != ::gateways::ItemValueType::NoValue ) {
                settingInfo->setValueType( valueType );
                setting->remove( common::resources::lua_events::ValueType );
                setting->insert( std::string( common::resources::tokens::ValueType ),
                                 std::make_unique<common::StringValue>(::gateways::toString( valueType ) ) );
            }

            // ElementType
            if( elementType ) {
                settingInfo->setElementType( elementType.value_or(::gateways::ItemValueType::NoValue ) );
            }

            // Description
            key = common::resources::tokens::Description;
            if( auto descriptionTable = common::iValueCast<common::TableValue>( ( *setting )[key] ) ) {
                std::string lang_tag;
                std::string text;
                if( ( *descriptionTable ).isKeyExist( "lang_tag" ) ) {
                    lang_tag = common::iValueCast<common::StringValue>( ( *descriptionTable )["lang_tag"] )->get();
                }
                if( ( *descriptionTable ).isKeyExist( "text" ) ) {
                    text = common::iValueCast<common::StringValue>( ( *descriptionTable )["text"] )->get();
                }
                settingInfo->setDescription( lang_tag, text );
            }

            // Enum
            key = common::resources::tokens::Enum;
            if( settingInfo->getValueType() == gateways::ItemValueType::Token ||
                settingInfo->getElementType() == gateways::ItemValueType::Token ) {
                auto enumTableValue = common::iValueCast<common::TableValue>( ( *setting )[key] );
                if( enumTableValue ) {
                    settingInfo->setEnumValues( enumTableValue );

                    auto enumTable = std::make_unique<common::TableValue>();
                    for( auto it = enumTableValue->begin(); it != enumTableValue->end(); ++it ) {
                        auto key1level = it->first;
                        if( key1level.empty() ) {
                            continue;
                        }
                        auto arr = common::iValueCast<common::TableValue>( it->second.get() );
                        if( !arr ) {
                            continue;
                        }

                        for( auto itAr = arr->begin(); itAr != arr->end(); ++itAr ) {
                            auto key2level = itAr->first;

                            if( key2level.empty() ) {
                                continue;
                            }

                            auto sec = common::iValueCast<common::StringValue>( itAr->second.get() );
                            if( !sec ) {
                                continue;
                            }

                            auto value = sec->get();

                            if( value.empty() ) {
                                continue;
                            }

                            auto entry = std::make_unique<common::TableValue>();
                            entry->insert( "lang_tag", std::make_unique<common::StringValue>( key2level ) );
                            entry->insert( "text", std::make_unique<common::StringValue>( value ) );
                            enumTable->insert( key1level, std::move( entry ) );
                        }
                    }
                    ( *setting ).remove( key );
                    ( *setting ).insert( key, std::move( enumTable ) );
                }
            }
            else if( settingInfo->getValueType() == gateways::ItemValueType::Int || settingInfo->isScalable() ) {
                // ValueMin ValueMax
                if( ( *setting ).isKeyExist( common::resources::lua_events::ValueMin ) &&
                    ( *setting ).isKeyExist( common::resources::lua_events::ValueMax ) ) {
                    auto minTable =
                        common::iValueCast<common::TableValue>( ( *setting )[common::resources::lua_events::ValueMin] );
                    auto maxTable =
                        common::iValueCast<common::TableValue>( ( *setting )[common::resources::lua_events::ValueMax] );
                    bool isNeedUpdateMinMax = false;
                    float valMin, valMax;

                    if( minTable && minTable->isKeyExist( common::resources::lua_events::Value ) ) {
                        auto newVal = settingInfo->castToItemType( std::move( minTable->clone() ) );
                        if( newVal ) {
                            common::ScalesConverterVisitor visitor(
                                ::core::HubSettingsInfo::temperatureScaleToStr(
                                    ::core::HubSettings::get().getTemperatureScale() ),
                                common::ScalesConverterVisitor::Precision );
                            newVal->accept( visitor );
                            auto ivalMin = common::iValueCast<common::FloatValue>( std::move( newVal ) );
                            if( ivalMin ) {
                                valMin = ivalMin->get();
                                isNeedUpdateMinMax = true;
                            }
                        }
                    }

                    if( maxTable && maxTable->isKeyExist( common::resources::lua_events::Value ) ) {
                        auto newVal = settingInfo->castToItemType( std::move( maxTable->clone() ) );
                        if( newVal ) {
                            common::ScalesConverterVisitor visitor(
                                ::core::HubSettingsInfo::temperatureScaleToStr(
                                    ::core::HubSettings::get().getTemperatureScale() ),
                                common::ScalesConverterVisitor::Precision );
                            newVal->accept( visitor );
                            auto ivalMax = common::iValueCast<common::FloatValue>( std::move( newVal ) );
                            if( ivalMax ) {
                                valMax = ivalMax->get();
                                isNeedUpdateMinMax = true;
                            }
                        }
                    }

                    if( isNeedUpdateMinMax ) {
                        settingInfo->setMinMax( { valMin, valMax } );
                    }
                }
            }

            // Info
            if( auto info =
                    common::iValueCast<common::TableValue>( ( *setting )[common::resources::lua_events::Info] ) ) {
                key = common::resources::lua_events::ZwaveParameterNumber;
                if( auto zwaveParameterNumber = common::iValueCast<common::IntValue>( ( *info )[key] ) ) {
                    settingInfo->setZwaveParameterNumber( zwaveParameterNumber->get() );
                }

                key = common::resources::lua_events::ZwaveParameterSize;
                if( auto zwaveParameterSize = common::iValueCast<common::IntValue>( ( *info )[key] ) ) {
                    settingInfo->setZwaveParameterSize( zwaveParameterSize->get() );
                }
            }

            // Descriptor
            key = common::resources::lua_events::Descriptor;
            if( auto descriptor = ( *setting )[key] ) {
                settingInfo->setDescriptor( descriptor->clone() );
            }

            setting->remove( common::resources::lua_events::Value );
            setting->remove( common::resources::lua_events::ValueDefault );
            setting->remove( common::resources::lua_events::ValueMin );
            setting->remove( common::resources::lua_events::ValueMax );
            setting->remove( common::resources::lua_events::DeviceId );
            setting->insert( std::string( common::resources::tokens::SubId ),
                             std::make_unique<common::StringValue>( settingId ) );
        }
        bus_command->m_setting = std::move( setting );
        m_ctx->getExecutor().send( NMA_NAME, std::move( bus_command ) );
    }
};

class AddGetewaySetting : public LuaBinding<std::string( std::unique_ptr<common::TableValue> )> {
public:
    static auto parseGetewayValueType(::common::TableValue& setting ) {
        namespace tokens = ::common::resources::lua_events;
        using ValueType = ::gateways::ItemValueType;
        using OptionalValueType = std::optional<ValueType>;
        auto valueTypeStr = ::common::iValueCast<::common::StringValue>( setting[tokens::ValueType] );
        if( !valueTypeStr ) {
            return std::make_pair( ValueType::NoValue, OptionalValueType{} );
        }
        else if( auto elementTypeStr = ::common::iValueCast<::common::StringValue>( setting[tokens::ElementType] ) ) {
            return std::make_pair(::gateways::toItemValueType( valueTypeStr->get() ),
                                  OptionalValueType{ ::gateways::toItemValueType( elementTypeStr->get() ) } );
        }
        else {
            return std::make_pair( gateways::toItemValueType( valueTypeStr->get() ), OptionalValueType{} );
        }
    }

    std::string operator()( std::unique_ptr<::common::TableValue> setting ) const {
        if( !setting ) {
            return {};
        }

        std::unique_ptr<gateways::GatewaySettingsInfo> settingInfo;

        std::string geteway_id = m_ctx->getPluginId();

        const auto [valueType, elementType] = parseGetewayValueType( *setting );

        gateways::SettingsInfo::LabelType label;
        if( auto infoTable =
                common::iValueCast<common::TableValue>( ( *setting )[common::resources::tokens::Label] ) ) {
            if( ( *infoTable ).isKeyExist( "lang_tag" ) ) {
                label.lang_tag = common::iValueCast<common::StringValue>( ( *infoTable )["lang_tag"] )->get();
            }
            if( ( *infoTable ).isKeyExist( "text" ) ) {
                label.text = common::iValueCast<common::StringValue>( ( *infoTable )["text"] )->get();
            }
        }
        if( geteway_id.empty() || valueType == gateways::ItemValueType::NoValue || label.text.empty() ) {
            return {};
        }

        std::string id =
            common::iValueCast<common::StringValue>( ( *setting )[common::resources::lua_events::Id] )->get();
        std::string settingId = geteway_id + "." + id;

        settingInfo =
            std::make_unique<gateways::GatewaySettingsInfo>( settingId, geteway_id, label, valueType, elementType );

        if( auto descriptionTable =
                common::iValueCast<common::TableValue>( ( *setting )[common::resources::tokens::Description] ) ) {
            std::string lang_tag;
            std::string text;

            if( ( *descriptionTable ).isKeyExist( "lang_tag" ) ) {
                lang_tag = common::iValueCast<common::StringValue>( ( *descriptionTable )["lang_tag"] )->get();
            }
            if( ( *descriptionTable ).isKeyExist( "text" ) ) {
                text = common::iValueCast<common::StringValue>( ( *descriptionTable )["text"] )->get();
            }
            settingInfo->setDescription( lang_tag, text );
        }

        if( settingInfo->getValueType() == gateways::ItemValueType::Token ||
            settingInfo->getElementType() == gateways::ItemValueType::Token ) {
            auto enumTableValue =
                common::iValueCast<common::TableValue>( ( *setting )[common::resources::tokens::Enum] );
            if( enumTableValue ) {
                settingInfo->setEnumValues( enumTableValue );
            }
            else {
                return {};
            }
        }
        else if( settingInfo->getValueType() == gateways::ItemValueType::Int || settingInfo->isScalable() ) {
            if( ( *setting ).isKeyExist( common::resources::lua_events::ValueMin ) &&
                ( *setting ).isKeyExist( common::resources::lua_events::ValueMax ) ) {
                auto minTable =
                    common::iValueCast<common::TableValue>( ( *setting )[common::resources::lua_events::ValueMin] );
                auto maxTable =
                    common::iValueCast<common::TableValue>( ( *setting )[common::resources::lua_events::ValueMax] );
                float min =
                    minTable && minTable->isKeyExist( common::resources::lua_events::Value ) ?
                        common::iValueCast<common::FloatValue>( ( *minTable )[common::resources::lua_events::Value] )
                            ->get() :
                        0.0;
                float max =
                    maxTable && maxTable->isKeyExist( common::resources::lua_events::Value ) ?
                        common::iValueCast<common::FloatValue>( ( *maxTable )[common::resources::lua_events::Value] )
                            ->get() :
                        0.0;
                settingInfo->setMinMax( { min, max } );
            }
        }

        auto value = ( *setting ).remove( "value" );

        value = settingInfo->castToItemType( std::move( value ) );

        auto& storage = ::storage::GatewaySettings::get();
        if( !storage.setInfo( *settingInfo, ::common::resources::lua_events::Synced, std::move( value ) ) ) {
            return {};
        }

        auto bus_command = std::make_unique<settings::responces::GatewaySettingAdded>();

        bus_command->m_settingInfo = std::move( settingInfo );

        m_ctx->getExecutor().send( NMA_NAME, std::move( bus_command ) );

        return settingId;
    }
};

class RemoveGatewaySetting : public LuaBinding<void( std::string )> {
public:
    void operator()( const std::string& settingId ) const {
        ::storage::GatewaySettings::get().remove( settingId );
        auto bus_command = std::make_unique<settings::responces::GatewaySettingRemoved>();
        bus_command->m_settingId = settingId;
        m_ctx->getExecutor().send( NMA_NAME, std::move( bus_command ) );
    }
};

class UpdateGatewaySettingStatus : public LuaBinding<void( std::string, std::string )> {
public:
    void operator()( std::string settingId, std::string newStatus ) const {
        auto& storage = ::storage::GatewaySettings::get();
        if( auto existingStatus = storage.getStatus( settingId );
            existingStatus != newStatus && storage.setStatus( settingId, newStatus ) ) {
            m_ctx->getExecutor().send(
                NMA_NAME, std::make_unique<settings::responces::GatewaySettingStatusUpdated>( settingId, newStatus ) );
        }
    }
};

class GetGatewaySettingIdsByDeviceId : public LuaBinding<std::vector<std::string>( std::string )> {
public:
    std::vector<std::string> operator()( std::string gatewayId ) const {
        std::vector<std::string> result;
        ::storage::GatewaySettings::get().enumerate(
            [&result, &gatewayId]( const ::gateways::GatewaySettingsInfo& settingInfo ) {
                if( settingInfo.getItemId() == gatewayId ) {
                    result.push_back( settingInfo.getId() );
                }
                return true;
            } );
        return result;
    }
};

class GetGatewaySetting : public LuaBinding<std::shared_ptr<::common::TableValue>( std::string )> {
public:
    std::shared_ptr<::common::TableValue> operator()( std::string settingId ) const {
        auto& storage = ::storage::GatewaySettings::get();
        auto settingInfo = storage.getInfo( settingId );
        if( !settingInfo ) {
            return {};
        }
        auto result = std::make_shared<::common::TableValue>();

        if( auto status = storage.getStatus( settingId ) ) {
            result->insert( lua_events::Status, std::make_unique<::common::StringValue>( *status ) );
        }

        if( auto value = storage.getValue( settingId ) ) {
            result->insert( lua_events::Value, std::move( value ) );
        }

        result->insert(::common::resources::lua_events::Id, std::make_unique<::common::StringValue>( settingId ) );

        result->insert(::common::resources::lua_events::GatewayId,
                       std::make_unique<::common::StringValue>( settingInfo->getItemId() ) );

        auto label = std::make_unique<::common::TableValue>();
        label->insert(::common::resources::lua_events::LangTag,
                      std::make_unique<::common::StringValue>( settingInfo->getLabel().lang_tag ) );

        label->insert(::common::resources::lua_events::Text,
                      std::make_unique<::common::StringValue>( settingInfo->getLabel().text ) );

        result->insert(::common::resources::lua_events::Label, std::move( label ) );

        if( settingInfo->isDescription() ) {
            auto description = std::make_unique<::common::TableValue>();
            description->insert(::common::resources::lua_events::LangTag,
                                std::make_unique<::common::StringValue>( settingInfo->getDescription().lang_tag ) );

            description->insert(::common::resources::lua_events::Text,
                                std::make_unique<::common::StringValue>( settingInfo->getDescription().text ) );

            result->insert(::common::resources::lua_events::Description, std::move( description ) );
        }

        result->insert(::common::resources::lua_events::ValueType,
                       std::make_unique<::common::StringValue>( gateways::toString( settingInfo->getValueType() ) ) );

        if( settingInfo->isMinMax() ) {
            result->insert(::common::resources::lua_events::ValueMin,
                           std::make_unique<::common::IntValue>( settingInfo->getMinMax().first ) );

            result->insert(::common::resources::lua_events::ValueMax,
                           std::make_unique<::common::IntValue>( settingInfo->getMinMax().second ) );
        }

        if( settingInfo->isEnum() ) {
            auto enumTable = std::make_unique<::common::TableValue>();
            for( size_t i = 0; i < settingInfo->getEnumItemsCount(); ++i ) {
                const auto& enumValue = settingInfo->getEnumValue( i );
                auto enumValueLabel = std::make_unique<::common::TableValue>();

                enumValueLabel->insert(::common::resources::lua_events::LangTag,
                                       std::make_unique<::common::StringValue>( enumValue.second.lang_tag ) );

                enumValueLabel->insert(::common::resources::lua_events::Text,
                                       std::make_unique<::common::StringValue>( enumValue.second.text ) );

                enumTable->insert( enumValue.first, std::move( enumValueLabel ) );
            }
            result->insert(::common::resources::lua_events::Enum, std::move( enumTable ) );
        }
        return result;
    }
};

class RemoveSetting : public LuaBinding<void( std::string )> {
public:
    void operator()( std::string settingId ) const {
        ::storage::DeviceSettings::get().remove( settingId );
        auto bus_command = std::make_unique<settings::responces::SettingRemoved>();
        bus_command->m_settingId = std::move( settingId );
        m_ctx->getExecutor().send( NMA_NAME, std::move( bus_command ) );
    }
};

class SetSettingValue : public LuaBinding<void( std::string, std::unique_ptr<::common::IValue>, std::string )> {
public:
    void operator()( std::string settingId, std::unique_ptr<::common::IValue> value, std::string newStatus ) const {
        if( settingId.empty() ) {
            return;
        }
        FUNC_LOGD( "core::set_setting_value( settingId=%s, value=%s, status=%s )",
                   settingId.c_str(),
                   value ? value->toJson().c_str() : "null",
                   newStatus.c_str() );

        auto settingInfo = ::storage::DeviceSettings::get().getInfo( settingId );
        if( !settingInfo ) {
            FUNC_LOGE( "Failed to create setting's info" );
            return;
        }

        auto& storage = ::storage::DeviceSettings::get();
        auto cast_value = settingInfo->castToItemType( std::move( value ) );
        if( cast_value ) {
            common::ScalesConverterVisitor visitor(
                ::core::HubSettingsInfo::temperatureScaleToStr(::core::HubSettings::get().getTemperatureScale() ),
                common::ScalesConverterVisitor::Precision );
            cast_value->accept( visitor );

            storage.setValue( settingId, cast_value->clone() );
        }

        auto bus_command = std::make_unique<settings::responces::SettingValueSetted>();
        bus_command->m_settingInfo = std::move( settingInfo );

        bus_command->m_value = std::move( cast_value );
        m_ctx->getExecutor().send( NMA_NAME, std::move( bus_command ) );

        if( auto existingStatus = storage.getStatus( settingId );
            existingStatus != newStatus && storage.setStatus( settingId, newStatus ) ) {
            m_ctx->getExecutor().send(
                NMA_NAME, std::make_unique<settings::responces::SettingStatusUpdated>( settingId, newStatus ) );
        }
    }
};

class SetGatewaySettingValue : public LuaBinding<void( std::string, std::unique_ptr<::common::IValue> )> {
public:
    void operator()( std::string settingId, std::unique_ptr<::common::IValue> value ) const {
        if( settingId.empty() ) {
            return;
        }

        auto settingInfo = ::storage::GatewaySettings::get().getInfo( settingId );
        if( !settingInfo ) {
            //   FUNC_LOGE( "Failed to create item's info" );
            return;
        }

        auto cast_value = settingInfo->castToItemType( std::move( value ) );
        if( cast_value ) {
            ::storage::GatewaySettings::get().setValue( settingId, cast_value->clone() );
        }

        auto bus_command = std::make_unique<settings::responces::GatewaySettingValueSetted>();
        bus_command->m_settingInfo = std::move( settingInfo );

        bus_command->m_value = std::move( cast_value );
        m_ctx->getExecutor().send( NMA_NAME, std::move( bus_command ) );
    }
};

class SetSettingDictionaryValue
: public LuaBinding<void( std::string, std::string, std::unique_ptr<common::TableValue> )> {
public:
    static const char* detectOperation( const ::common::TableValue* existingItemValue,
                                        const ::common::IValue* newDictValue,
                                        const std::string& key ) {
        if( !newDictValue && existingItemValue && existingItemValue->isKeyExist( key ) ) {
            return tokens::Removed;
        }
        else if( newDictValue && ( !existingItemValue || !existingItemValue->isKeyExist( key ) ) ) {
            return tokens::Added;
        }
        else if( newDictValue && existingItemValue && existingItemValue->isKeyExist( key ) &&
                 *( *existingItemValue )[key] != *newDictValue ) {
            return tokens::Updated;
        }
        return nullptr;
    }

    static std::unique_ptr<::common::TableValue> getUpdatedItemValue( const ::gateways::SettingsInfo& setting,
                                                                      const ::common::TableValue* existingItemValue,
                                                                      const ::common::IValue* newDictValue,
                                                                      const std::string& key ) {
        auto updatedItemValue = ::common::iValueCast<::common::TableValue>(
            existingItemValue ? existingItemValue->clone() : setting.createDefaultValue() );
        if( newDictValue ) {
            updatedItemValue->insert( key, newDictValue->clone() );
        }
        else {
            updatedItemValue->remove( key );
        }
        return updatedItemValue;
    }

    void operator()( std::string settingId, std::string key, std::unique_ptr<::common::TableValue> dictValue ) const {
        FUNC_LOGD( "core::set_setting_dictionary_value( settingId=%s, key=%s, value=%s )",
                   settingId.c_str(),
                   key.c_str(),
                   dictValue ? dictValue->toJson( "value" ).c_str() : "null" );

        auto settingInfo = ::storage::DeviceSettings::get().getInfo( settingId );
        if( !settingInfo ) {
            FUNC_LOGE( "Cannot to find setting info, id=%s", settingId.c_str() );
            return;
        }
        if( dictValue ) {
            dictValue =
                ::common::iValueCast<::common::TableValue>( settingInfo->castToItemType( std::move( dictValue ) ) );
            if( !dictValue ) {
                FUNC_LOGE( "Cannot cast value to item type" );
                return;
            }
        }
        auto existingItemValue =
            ::common::iValueCast<::common::TableValue>(::storage::DeviceSettings::get().getValue( settingId ) );
        auto updatedItemValue = getUpdatedItemValue( *settingInfo, existingItemValue.get(), dictValue.get(), key );
        if( auto operation = detectOperation( existingItemValue.get(), dictValue.get(), key ) ) {
            ::storage::DeviceSettings::get().setValue( settingId, std::move( updatedItemValue ) );
            m_ctx->getExecutor().send<settings::responces::SettingDictionaryValueUpdated>(
                NMA_NAME, std::move( settingInfo ), std::move( dictValue ), std::move( key ), operation );
        }
    }
};

class UpdateSettingStatus : public LuaBinding<void( std::string, std::string )> {
public:
    void operator()( std::string settingId, std::string newStatus ) const {
        FUNC_LOGD( "core::update_setting_status( settingId=%s, status=%s )", settingId.c_str(), newStatus.c_str() );
        auto& storage = ::storage::DeviceSettings::get();
        if( auto existingStatus = storage.getStatus( settingId );
            existingStatus != newStatus && storage.setStatus( settingId, newStatus ) ) {
            m_ctx->getExecutor().send(
                NMA_NAME, std::make_unique<settings::responces::SettingStatusUpdated>( settingId, newStatus ) );
        }
    }
};

class UpdateSettingDictionaryStatus : public LuaBinding<void( std::string, std::string, std::string )> {
public:
    void operator()( std::string settingId, std::string elementId, std::string newStatus ) {
        FUNC_LOGD( "code::update_setting_dictionary_status( settingId=%s, elementId=%s, status=%s )",
                   settingId.c_str(),
                   elementId.c_str(),
                   newStatus.c_str() );
        auto& storage = ::storage::DeviceSettings::get();
        if( auto existingValue = ::common::iValueCast<::common::TableValue>( storage.getValue( settingId ) ) ) {
            if( auto element = ::common::iValueCast<::common::TableValue>( ( *existingValue )[elementId] ) ) {
                if( element->isKeyExist( lua_events::Status ) ) {
                    if( auto existingStatus =
                            ::common::iValueCast<::common::StringValue>( ( *element )[lua_events::Status] );
                        existingStatus && existingStatus->get() == newStatus ) {
                        return;
                    }
                }
                element->insert( lua_events::Status, ::common::IValue::create( newStatus ) );
                if(::storage::DeviceSettings::get().setValue( settingId, std::move( existingValue ) ) ) {
                    m_ctx->getExecutor().send( NMA_NAME,
                                               std::make_unique<settings::responces::SettingDictionaryStatusUpdated>(
                                                   settingId, elementId, newStatus ) );
                }
            }
        }
    }
};

class GetSettingIdsByDeviceId : public LuaBinding<std::vector<std::string>( std::string )> {
public:
    std::vector<std::string> operator()( std::string deviceId ) const {
        FUNC_LOGD( "core::get_setting_ids_by_device_id( deviceId=%s )", deviceId.c_str() );
        std::vector<std::string> result;
        ::storage::DeviceSettings::get().enumerate(
            [&result, &deviceId]( const ::gateways::SettingsInfo& settingInfo ) {
                if( settingInfo.getItemId() == deviceId ) {
                    result.push_back( settingInfo.getId() );
                }
                return true;
            } );
        return result;
    }
};

class GetSetting : public LuaBinding<std::shared_ptr<::common::TableValue>( std::string )> {
public:
    std::shared_ptr<::common::TableValue> operator()( std::string settingId ) const {
        FUNC_LOGD( "core::get_setting( settingId=%s )", settingId.c_str() );
        auto& storage = ::storage::DeviceSettings::get();
        auto settingInfo = storage.getInfo( settingId );
        if( !settingInfo ) {
            FUNC_LOGE( "Cannot find device setting info, id=%s", settingId.c_str() );
            return {};
        }

        auto result = std::make_shared<::common::TableValue>();

        if( auto status = storage.getStatus( settingId ) ) {
            result->insert( lua_events::Status, ::common::IValue::create( *status ) );
        }
        if( auto value = storage.getValue( settingId ) ) {
            result->insert(::common::resources::lua_events::Value, std::move( value ) );
        }
        else {
            result->insert(::common::resources::lua_events::Value, std::make_unique<::common::TableValue>() );
        }

        result->insert(::common::resources::lua_events::Id, std::make_unique<::common::StringValue>( settingId ) );
        result->insert(::common::resources::lua_events::DeviceId,
                       std::make_unique<::common::StringValue>( settingInfo->getItemId() ) );

        auto label = std::make_unique<::common::TableValue>();
        label->insert(::common::resources::lua_events::LangTag,
                      std::make_unique<::common::StringValue>( settingInfo->getLabel().lang_tag ) );
        label->insert(::common::resources::lua_events::Text,
                      std::make_unique<::common::StringValue>( settingInfo->getLabel().text ) );
        result->insert(::common::resources::lua_events::Label, std::move( label ) );

        if( settingInfo->isDescription() ) {
            auto description = std::make_unique<::common::TableValue>();
            description->insert(::common::resources::lua_events::LangTag,
                                std::make_unique<::common::StringValue>( settingInfo->getDescription().lang_tag ) );
            description->insert(::common::resources::lua_events::Text,
                                std::make_unique<::common::StringValue>( settingInfo->getDescription().text ) );
            result->insert(::common::resources::lua_events::Description, std::move( description ) );
        }

        result->insert(::common::resources::lua_events::ValueType,
                       std::make_unique<::common::StringValue>( gateways::toString( settingInfo->getValueType() ) ) );

        if( settingInfo->isMinMax() ) {
            result->insert(::common::resources::lua_events::ValueMin,
                           std::make_unique<::common::IntValue>( settingInfo->getMinMax().first ) );
            result->insert(::common::resources::lua_events::ValueMax,
                           std::make_unique<::common::IntValue>( settingInfo->getMinMax().second ) );
        }
        if( settingInfo->isEnum() ) {
            auto enumTable = std::make_unique<::common::TableValue>();
            for( size_t i = 0; i < settingInfo->getEnumItemsCount(); ++i ) {
                const auto& enumValue = settingInfo->getEnumValue( i );
                auto enumValueLabel = std::make_unique<::common::TableValue>();

                enumValueLabel->insert(::common::resources::lua_events::LangTag,
                                       std::make_unique<::common::StringValue>( enumValue.second.lang_tag ) );
                enumValueLabel->insert(::common::resources::lua_events::Text,
                                       std::make_unique<::common::StringValue>( enumValue.second.text ) );
                enumTable->insert( enumValue.first, std::move( enumValueLabel ) );
            }
            result->insert(::common::resources::lua_events::Enum, std::move( enumTable ) );
        }

        if( settingInfo->hasZwaveParameters() ) {
            auto info = std::make_unique<::common::TableValue>();
            info->insert(::common::resources::lua_events::ZwaveParameterNumber,
                         std::make_unique<::common::IntValue>( settingInfo->getZwaveParameterNumber() ) );
            info->insert(::common::resources::lua_events::ZwaveParameterSize,
                         std::make_unique<::common::IntValue>( settingInfo->getZwaveParameterSize() ) );
            result->insert(::common::resources::lua_events::Info, std::move( info ) );
        }
        return result;
    }
};

class Subscribe : public Overloaded<LuaBinding<std::string( std::string, std::unique_ptr<::common::TableValue> )>,
                                    LuaBinding<std::string( std::string )>> {
public:
    std::string operator()( std::string scriptName, std::unique_ptr<::common::TableValue> eventFilter ) const {
        FUNC_LOGD(
            "core::subscribe( scriptName=%s, eventFilter=%s )", scriptName.c_str(), eventFilter->toJson().c_str() );
        m_ctx->getExecutor().send<bus::command::SetHandler>(
            ::bus::lua_event::BUS_NAME, m_ctx->getExecutor().addr(), scriptName );
        return "exclusive_core_subscriber";
    }

    std::string operator()( std::string scriptName ) const {
        FUNC_LOGD( "core::subscribe( scriptName=%s )", scriptName.c_str() );
        m_ctx->getExecutor().send<bus::command::SetHandler>(
            ::bus::lua_event::BUS_NAME, m_ctx->getExecutor().addr(), scriptName );
        return "exclusive_core_subscriber";
    }
};

class SetSubscriptionFilters : public LuaBinding<void( std::string, std::unique_ptr<::common::ArrayValue> )> {
public:
    void operator()( std::string subscriberId, std::unique_ptr<::common::ArrayValue> filterRules ) const {
        FUNC_LOGD( "core::set_subscription_filters( subscriberId=%s, filterRules=%s )",
                   subscriberId.c_str(),
                   filterRules->toJson().c_str() );
    }
};

class GetGateways : public LuaBinding<std::shared_ptr<::common::ArrayValue>()> {
public:
    std::shared_ptr<::common::ArrayValue> operator()() {
        FUNC_LOGD( "core::get_gateways()" );
        auto gatewayList = std::make_unique<::common::ArrayValue>();
        const auto& storage = m_ctx->getGatewayUserDataStorage();
        ::gateways::enumerateGateways( [&gatewayList, &storage](::gateways::GatewayID id, ::gateways::IGateway* ) {
            if( auto gatewayFields = storage.getUserData( id ) ) {
                ::gateways::utils::convertGatewayUserDataFieldNamesToLuaNames( *gatewayFields );
                gatewayList->push_back( std::move( gatewayFields ) );
            }
            return true;
        } );
        return gatewayList;
    }
};

class GetGateway : public LuaBinding<std::shared_ptr<::common::TableValue>()> {
public:
    std::shared_ptr<::common::TableValue> operator()() {
        FUNC_LOGD( "core::get_gateway()" );
        auto pluginId = m_ctx->getPluginId();
        if( auto gatewayFields =
                m_ctx->getGatewayUserDataStorage().getUserData(::gateways::toGatewayID( pluginId.c_str() ) ) ) {
            ::gateways::utils::convertGatewayUserDataFieldNamesToLuaNames( *gatewayFields );
            return gatewayFields;
        }
        return std::make_shared<::common::TableValue>(::common::TableValue() );
    }
};

class UpdateGateway : public LuaBinding<void( std::unique_ptr<::common::TableValue> )> {
public:
    void operator()( std::unique_ptr<::common::TableValue> newFields ) {
        FUNC_LOGD( "core::update_gateway( gateway=%s )", newFields->toJson().c_str() );
        if( newFields->empty() ) {
            return;
        }
        ::gateways::utils::convertGatewayUserDataFieldNamesFromLuaNames( *newFields );
        auto pluginId = m_ctx->getPluginId();
        m_ctx->getExecutor().send( NMA_NAME,
                                   std::make_unique<::bus::command::UpdateGateway>(
                                       ::gateways::toGatewayID( pluginId.c_str() ),
                                       ::common::iValueCast<::common::TableValue>( newFields->clone() ) ) );
        m_ctx->getGatewayUserDataStorage().setUserData(::gateways::toGatewayID( pluginId.c_str() ),
                                                       std::move( newFields ) );
    }
};

class SendDeviceFirmwareDownloadBroadcast : public LuaBinding<void( std::unique_ptr<common::TableValue> )> {
public:
    void operator()( std::unique_ptr<::common::TableValue> value ) {
        FUNC_LOGD( "core::update_gateway( gateway=%s )", value->toJson().c_str() );
        if( value->empty() ) {
            return;
        }

        std::string image =
            common::iValueCast<common::StringValue>( ( *value )[common::resources::lua_events::Image] )->get();
        std::string checksum =
            common::iValueCast<common::StringValue>( ( *value )[common::resources::lua_events::Checksum] )->get();
        std::string status =
            common::iValueCast<common::StringValue>( ( *value )[common::resources::lua_events::Status] )->get();
        int progress =
            common::iValueCast<common::IntValue>( ( *value )[common::resources::lua_events::Progress] )->get();

        auto progressStr = std::to_string( progress );
        auto command = std::make_unique<::lua::responce::HubSendBroadcast>();
        if( command ) {
            command->m_event.assign( common::resources::broadcasts::events::NodeFirmwareDownloadStatus );
            command->m_value.push_back( image );
            command->m_value.push_back( checksum );
            command->m_value.push_back( status );
            command->m_value.push_back( progressStr );
            m_ctx->getExecutor().send(::lua::responce::BUS_NAME, std::move( command ) );
        }
    }
};

class SendDeviceFirmwareUpdateBroadcast : public LuaBinding<void( std::unique_ptr<common::TableValue> )> {
public:
    void operator()( std::unique_ptr<::common::TableValue> value ) {
        FUNC_LOGD( "core::update_gateway( gateway=%s )", value->toJson().c_str() );
        if( value->empty() ) {
            return;
        }

        std::string deviceId =
            common::iValueCast<common::StringValue>( ( *value )[common::resources::lua_events::DeviceId] )->get();
        std::string firmwareId =
            common::iValueCast<common::StringValue>( ( *value )[common::resources::lua_events::FirmwareId] )->get();
        std::string status =
            common::iValueCast<common::StringValue>( ( *value )[common::resources::lua_events::Status] )->get();
        int progress =
            common::iValueCast<common::IntValue>( ( *value )[common::resources::lua_events::Progress] )->get();
        auto progressStr = std::to_string( progress );

        auto command = std::make_unique<::lua::responce::HubSendBroadcast>();
        if( command ) {
            command->m_event.assign( common::resources::broadcasts::events::NodeFirmwareUpdateStatus );
            command->m_value.push_back( deviceId );
            command->m_value.push_back( firmwareId );
            command->m_value.push_back( status );
            command->m_value.push_back( progressStr );

            m_ctx->getExecutor().send(::lua::responce::BUS_NAME, std::move( command ) );
        }
    }
};

class UpdateDeviceItemsStatus : public LuaBinding<bool( std::string deviceId, std::string status )> {
public:
    bool operator()( std::string deviceId, std::string newStatus ) {
        FUNC_LOGI( "update_device_items_status( deviceid=%s, status=%s )", deviceId.c_str(), newStatus.c_str() );
        if( gateways::ItemInfo::stringToStatus( newStatus.c_str() ) == gateways::ItemInfo::Status::unknown ) {
            return false;
        }
        auto f = [&newStatus, ctx = m_ctx]( const char* itemId ) {
            if( auto itemInfo = ::gateways::ItemInfo::create( itemId ) ) {
                itemInfo->setStatus( newStatus );
                ctx->getExecutor().send<::lua::responce::UpdateItem>(
                    ::lua::responce::BUS_NAME,
                    std::move( itemInfo ),
                    std::move(::cache::Cache::get().getValueByKey( itemId ) ) );
            }
        };
        lua::module::Core::processItemsByDeviceId( deviceId.c_str(), f );
        return true;
    }
};

class UpdateItemValue
: public Overloaded<LuaBinding<void( std::string, std::unique_ptr<::common::IValue> )>,
                    LuaBinding<void( std::string, std::unique_ptr<::common::IValue>, std::string )>> {
public:
    static bool isEmptyCollection(::common::IValue* value ) {
        if( !value ) {
            return false;
        }
        if( auto arrayValue = ::common::iValueCast<::common::ArrayValue>( value );
            arrayValue && arrayValue->get().empty() ) {
            return true;
        }
        if( auto tableValue = ::common::iValueCast<::common::TableValue>( value );
            tableValue && tableValue->get().empty() ) {
            return true;
        }
        return false;
    }

    static bool isCollection(::gateways::ItemValueType type ) {
        return ::gateways::ItemValueType::Dictionary == type || ::gateways::ItemValueType::Array == type;
    }

    void impl( const std::string& itemId,
               std::unique_ptr<::common::IValue> newValue,
               const std::string& newStatus ) const {
        auto itemInfo = gateways::ItemInfo::create( itemId );
        if( !itemInfo ) {
            FUNC_LOGE( "Item %s not found", itemId.c_str() );
            return;
        }
        if( isEmptyCollection( newValue.get() ) && isCollection( itemInfo->getValueType() ) ) {
            newValue = itemInfo->createDefaultValue();
        }
        auto value = itemInfo->castToItemType( std::move( newValue ) );
        if( !value ) {
            FUNC_LOGE( "Cannot cast item value to item type" );
            return;
        }
        auto cacheStatus = itemInfo->getStatus();
        if( auto cacheValue = itemInfo->getValueCache();
            cacheValue && ( *value == *cacheValue ) && cacheStatus == newStatus ) {
            return;
        }
        itemInfo->setValue( value->clone() );
        itemInfo->setStatus( newStatus );
        Core::sendReachableState( m_ctx->getExecutor(), itemInfo->getDeviceId().c_str(), true );
        m_ctx->getExecutor().send<::lua::responce::UpdateItem>(
            ::lua::responce::BUS_NAME, std::move( itemInfo ), std::move( value ) );
    }

    void operator()( std::string itemId, std::unique_ptr<::common::IValue> newValue ) const {
        FUNC_LOGI( "update_item_value(item_id=%s, value=%s, [defaultNewStatus=synced])",
                   itemId.c_str(),
                   newValue->toJson().c_str() );
        impl( itemId, std::move( newValue ), ::common::resources::lua_events::Synced );
    }

    void operator()( std::string itemId, std::unique_ptr<::common::IValue> newValue, std::string newStatus ) const {
        FUNC_LOGI( "update_item_value(item_id=%s, value=%s, newStatus=%s)",
                   itemId.c_str(),
                   newValue->toJson().c_str(),
                   newStatus.c_str() );
        impl( itemId, std::move( newValue ), newStatus );
    }
};

class UpdateItemValueWithMinMax : public LuaBinding<void( std::string,
                                                          std::unique_ptr<::common::IValue>,
                                                          std::unique_ptr<::common::IValue>,
                                                          std::unique_ptr<::common::IValue> )> {
public:
    void operator()( std::string itemId,
                     std::unique_ptr<::common::IValue> value,
                     std::unique_ptr<::common::IValue> valueMin,
                     std::unique_ptr<::common::IValue> valueMax ) {
        FUNC_LOGI( "update_item_value_with_min_max(item_id=%s, value=%s, valueMin=%s, valueMax=%s)",
                   itemId.c_str(),
                   value->toJson().c_str(),
                   valueMin->toJson().c_str(),
                   valueMax->toJson().c_str() );
        auto itemInfo = gateways::ItemInfo::create( itemId );
        if( !itemInfo ) {
            FUNC_LOGE( "Item %s not found", itemId.c_str() );
            return;
        }
        bool isNeedUpdateMinMax = false;

        float valMin, valMax;
        std::tie( valMin, valMax ) = itemInfo->getMinMax();
        if( valueMin ) {
            auto newVal = itemInfo->castToItemType( valueMin->clone() );
            if( newVal ) {
                common::ScalesConverterVisitor visitor(
                    ::core::HubSettingsInfo::temperatureScaleToStr(::core::HubSettings::get().getTemperatureScale() ),
                    common::ScalesConverterVisitor::Precision );
                newVal->accept( visitor );
                auto ivalMin = common::iValueCast<common::FloatValue>( std::move( newVal ) );
                if( ivalMin ) {
                    if( ivalMin->get() != valMin ) {
                        valMin = ivalMin->get();
                        isNeedUpdateMinMax = true;
                    }
                }
            }
        }

        if( valueMax ) {
            auto newVal = itemInfo->castToItemType( valueMax->clone() );
            if( newVal ) {
                common::ScalesConverterVisitor visitor(
                    ::core::HubSettingsInfo::temperatureScaleToStr(::core::HubSettings::get().getTemperatureScale() ),
                    common::ScalesConverterVisitor::Precision );
                newVal->accept( visitor );
                auto ivalMax = common::iValueCast<common::FloatValue>( std::move( newVal ) );
                if( ivalMax ) {
                    if( ivalMax->get() != valMax ) {
                        valMax = ivalMax->get();
                        isNeedUpdateMinMax = true;
                    }
                }
            }
        }

        if( isNeedUpdateMinMax ) {
            itemInfo->setMinMax( { valMin, valMax } );
            gateways::ObjectSerializer serializer;
            auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
            auto buff = serializer.serialize( *itemInfo );
            !buff.empty() && store->setBlob( itemInfo->getId(), buff );
        }
        UpdateItemValue updateItem;
        updateItem.setContext( m_ctx );
        updateItem.impl( itemId, std::move( value ), ::common::resources::lua_events::Synced );
    }
};

class GetItemDictionaryValue : public LuaBinding<std::shared_ptr<::common::IValue>( std::string, std::string )> {
public:
    std::shared_ptr<::common::IValue> operator()( std::string itemId, std::string dictionaryId ) const {
        FUNC_LOGD( "core::get_item_dictionary_value( itemId Id = %s, dictionaryId = %s )",
                   itemId.c_str(),
                   dictionaryId.c_str() );

        auto cacheValue = cache::Cache::get().getValueByKey( itemId );
        if( cacheValue ) {
            auto table = common::iValueCast<common::TableValue>( cacheValue.get() );
            if( table ) {
                if( ( *table ).isKeyExist( dictionaryId ) ) {
                    return std::shared_ptr<::common::IValue>( ( *table )[dictionaryId]->clone() );
                }
            }
        }

        return {};
    }
};

class GetRootDeviceId : public LuaBinding<std::string( std::string )> {
public:
    std::string operator()( std::string childId ) {
        FUNC_LOGD( "core::get_root_device_id( Id=%s )", childId.c_str() );
        auto deviceInfo = gateways::BaseDeviceInfo::create( childId );
        if( !deviceInfo ) {
            FUNC_LOGE( "Failed to create device info" );
            return {};
        }

        return strlen( deviceInfo->getParentDeviceId() ) > 0 ? deviceInfo->getParentDeviceId() : childId.c_str();
    }
};

class CalculateTimeWithDelay : public LuaBinding<std::string( std::string )> {
public:
    std::string operator()( std::string delay ) {
        FUNC_LOGD( "core::calculate_time_with_delay( delay=%s )", delay.c_str() );

        auto currectDate = ::TimeUtils::localTimeGetter();

        auto tm = ::TimeUtils::stringToTimeStruct( delay, "%H:%M:%S" );

        currectDate.addHours( tm.tm_hour );
        currectDate.addMinutes( tm.tm_min );
        currectDate.addSeconds( tm.tm_sec );

        return currectDate.toString( "%FT%T" );
    }
};

class IsInsideWeekInterval : public LuaBinding<bool( std::vector<std::string>, std::string, std::string )> {
public:
    bool operator()( std::vector<std::string> weekDays, std::string startTime, std::string endTime ) {
        FUNC_LOGD(
            "core::is_inside_week_interval( start_time=%s, end_time=%s ), days:", startTime.c_str(), endTime.c_str() );
        for( auto days : weekDays )
            FUNC_LOGD( "%s", days.c_str() );

        auto weekdaysSet = ::TimeUtils::convertWeekdays( weekDays );
        auto currectDate = ::TimeUtils::localTimeGetter();

        // Wrong week day
        if( weekdaysSet.find( currectDate.getWeekday() ) == weekdaysSet.end() ) {
            return false;
        }

        // Check time intervals
        GregorianCalendar::Date startDate = ::TimeUtils::getDateWithModifiedTime( currectDate, startTime );
        GregorianCalendar::Date endDate = ::TimeUtils::getDateWithModifiedTime( currectDate, endTime );

        return ( startDate <= currectDate ) && ( currectDate <= endDate );
    }
};

class GetNextWeekEvent : public LuaBinding<std::string( std::vector<std::string>, std::string )> {
public:
    std::string operator()( std::vector<std::string> weekDays, std::string time ) {
        FUNC_LOGD( "core::is_inside_week_interval( time=%s ), days:", time.c_str() );
        for( auto days : weekDays )
            FUNC_LOGD( "%s", days.c_str() );

        auto weekdaysSet = ::TimeUtils::convertWeekdays( weekDays );
        auto currectDate = ::TimeUtils::localTimeGetter();

        GregorianCalendar::Date targetDate = ::TimeUtils::getDateWithModifiedTime( currectDate, time );
        for( int i = 0; i <= 1; ++i, targetDate.incrementWeek() ) {
            for( const auto& weekday : weekdaysSet ) {
                targetDate.moveToWeekday( weekday );

                if( currectDate < targetDate ) {
                    return targetDate.toString( "%FT%T" );
                }
            }
        }

        return std::string{};
    }
};

class GetLocalTime : public Overloaded<LuaBinding<std::string( std::string )>, LuaBinding<std::string()>> {
public:
    std::string operator()() {
        return operator()( "%FT%T%z" );
    }

    std::string operator()( std::string format ) {
        auto now = time( nullptr );
        struct tm timeInfo {};
        localtime_r( &now, &timeInfo );
        return TimeUtils::timeStructToString( timeInfo, format );
    }
};

class NotifyProcessStarted : public LuaBinding<void( std::string )> {
public:
    void operator()( std::string processString ) const {
        auto pluginId = m_ctx->getPluginId();
        FUNC_LOGD( "Need impliment NotifyProcessStarted pluginId = %s!!!!!", pluginId.c_str() );

        // TODO Need impliment NotifyProcessStarted

        return;
    }
};

class NotifyProcessStopped : public LuaBinding<void( std::string, std::string )> {
public:
    void operator()( std::string processString, std::string status ) const {
        auto pluginId = m_ctx->getPluginId();
        FUNC_LOGD( "Need impliment NotifyProcessStopped pluginId = %s!!!!!", pluginId.c_str() );

        // TODO Need impliment NotifyProcessStopped

        return;
    }
};

}  // namespace core
}  // namespace bindings
}  // namespace module
}  // namespace lua
