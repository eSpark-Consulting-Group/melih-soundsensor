#include <LuaModuleCore.h>
#include <Lua.h>
#include <GatewayUtils.h>
#include <DeviceSettingsManager.h>
#include <Utils.h>
#include <HardUtils.h>
#include <DebugUtils.h>
#include "IDInfo.h"

#include <GatewayID.h>
#include <IGateway.h>
#include <DeviceInfo.h>
#include <SettingsInfo.h>
#include "DeviceSettingsInfo.hpp"
#include "GatewaySettingsInfo.hpp"
#include <ObjectSerializer.h>

#include "lua/responce/AddDevice.h"
#include "lua/responce/UpdateDevice.h"
#include "lua/responce/RemoveDevice.h"
#include <BusCommandRemoveItem.h>
#include "lua/responce/AddItem.h"
#include "lua/responce/UpdateItem.h"
#include "lua/responce/UpdateItemDictionary.h"
#include "lua/responce/HubSendBroadcast.h"
#include <BusCommandSetHandler.h>
#include <BusCommandHandleEventUpdateProductName.hpp>
#include <BusCommandUpdateGateway.h>
#include <BusCommandUpdateGateway.h>
#include <BusCommandHubSettingsApply.hpp>
#include <BusCommandNmaClientClbk.h>

#include <CommonResources.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>

#include <IValue.h>
#include <IValueCast.h>
#include <IValueVisitor.h>
#include <Cache.h>

#include "values/ArrayValue.h"
#include "values/TableValue.h"

#include "bindings/LuaBindingHelper.h"
#include "bindings/LuaCoreBindings.hpp"

#include "converters/ScalesConverterVisitor.hpp"
#include "HubSettings.h"

//#define SIMPLE_MODTMR_DEBUG
#define MODCOR_LOGE( ... ) ESP_LOGE(::esp_log_tags::CORE, __VA_ARGS__ )

#ifdef SIMPLE_MODCOR_DEBUG

#include <esp_log.h>
#include "EspLogTags.h"

#define MODCOR_LOGI( ... ) ESP_LOGI(::esp_log_tags::CORE, __VA_ARGS__ )
//#define MODCOR_LOGE( ... ) ESP_LOGE(::esp_log_tags::CORE, __VA_ARGS__ )
#define MODCOR_LOGW( ... ) ESP_LOGW(::esp_log_tags::CORE, __VA_ARGS__ )
#define MODCOR_LOGD( ... ) ESP_LOGD(::esp_log_tags::CORE, __VA_ARGS__ )

#else

#define MODCOR_LOGD( ... ) \
    do {                   \
    } while( false )
#define MODCOR_LOGI( ... ) \
    do {                   \
    } while( false )
#define MODCOR_LOGW( ... ) \
    do {                   \
    } while( false )

#endif

namespace lua {

namespace module {

using namespace storage;
using BlobType = storage::KeyValueStorage::BlobType;

bool Core::initialize() {
    return true;
}

Core::Core( Executor& exec, const char* name )
: IModule( exec, name ) {}

int Core::regModule( lua_State* lua ) {
    static const auto bindArray = lua::createBindingsMap(
        luaL_Reg{ "subscribe", bindings::luaBinding<bindings::core::Subscribe> },
        luaL_Reg{ "set_subscription_filters", bindings::luaBinding<bindings::core::SetSubscriptionFilters> },

        luaL_Reg{ "add_device", Core::luaAddDevice },
        luaL_Reg{ "add_item", Core::luaAddItem },
        luaL_Reg{ "add_setting", bindings::luaBinding<bindings::core::AddSetting> },
        luaL_Reg{ "add_gateway_setting", bindings::luaBinding<bindings::core::AddGetewaySetting> },

        luaL_Reg{ "calculate_time_with_delay", bindings::luaBinding<bindings::core::CalculateTimeWithDelay> },

        luaL_Reg{ "get_local_time", bindings::luaBinding<bindings::core::GetLocalTime> },
        luaL_Reg{ "get_gateway", bindings::luaBinding<bindings::core::GetGateway> },
        luaL_Reg{ "get_gateways", bindings::luaBinding<bindings::core::GetGateways> },
        luaL_Reg{ "get_device", Core::luaGetDevice },
        luaL_Reg{ "get_devices", Core::luaGetDevices },
        luaL_Reg{ "get_item", Core::luaGetItem },
        luaL_Reg{ "get_item_dictionary_value", bindings::luaBinding<bindings::core::GetItemDictionaryValue> },
        luaL_Reg{ "get_items", Core::luaGetItems },
        luaL_Reg{ "get_items_by_device_id", Core::luaGetItemsByDeviceId },
        luaL_Reg{ "generate_item_dictionary_number_id", Core::luaGenerateItemDictionaryNumberId },
        luaL_Reg{ "get_product_name", Core::luaGetProductName },
        luaL_Reg{ "get_root_device_id", bindings::luaBinding<bindings::core::GetRootDeviceId> },
        luaL_Reg{ "get_setting", bindings::luaBinding<bindings::core::GetSetting> },
        luaL_Reg{ "get_gateway_setting", bindings::luaBinding<bindings::core::GetGatewaySetting> },
        luaL_Reg{ "get_setting_ids_by_device_id", bindings::luaBinding<bindings::core::GetSettingIdsByDeviceId> },
        luaL_Reg{ "get_gateway_setting_ids_by_gateway_id",
                  bindings::luaBinding<bindings::core::GetGatewaySettingIdsByDeviceId> },
        luaL_Reg{ "get_next_week_event", bindings::luaBinding<bindings::core::GetNextWeekEvent> },

        luaL_Reg{ "is_inside_week_interval", bindings::luaBinding<bindings::core::IsInsideWeekInterval> },

        luaL_Reg{ "modify_device", Core::luaModifyDevice },
        luaL_Reg{ "modify_item", Core::luaModifyItem },
        luaL_Reg{ "modify_setting", bindings::luaBinding<bindings::core::ModifySetting> },

        luaL_Reg{ "notify_process_started", bindings::luaBinding<bindings::core::NotifyProcessStarted> },
        luaL_Reg{ "notify_process_stopped", bindings::luaBinding<bindings::core::NotifyProcessStopped> },

        luaL_Reg{ "remove_gateway_devices", Core::luaRemoveGatewayDevices },
        luaL_Reg{ "remove_gateway_setting", bindings::luaBinding<bindings::core::RemoveGatewaySetting> },
        luaL_Reg{ "remove_device", Core::luaRemoveDevice },
        luaL_Reg{ "remove_device_sync", Core::luaRemoveDevice },
        luaL_Reg{ "remove_item", Core::luaRemoveItem },
        luaL_Reg{ "remove_item_dictionary_value", Core::luaRemoveItemDictionaryValue },
        luaL_Reg{ "remove_setting", bindings::luaBinding<bindings::core::RemoveSetting> },

        luaL_Reg{ "send_device_firmware_update_broadcast",
                  bindings::luaBinding<bindings::core::SendDeviceFirmwareUpdateBroadcast> },
        luaL_Reg{ "send_device_firmware_download_broadcast",
                  bindings::luaBinding<bindings::core::SendDeviceFirmwareDownloadBroadcast> },
        luaL_Reg{ "send_ui_broadcast", Core::luaSendUiBroadcast },
        luaL_Reg{ "set_gateway_setting_value", bindings::luaBinding<bindings::core::SetGatewaySettingValue> },
        luaL_Reg{ "set_gateway_setting_status", bindings::luaBinding<bindings::core::UpdateGatewaySettingStatus> },
        luaL_Reg{ "set_setting_dictionary_value", bindings::luaBinding<bindings::core::SetSettingDictionaryValue> },
        luaL_Reg{ "set_setting_value", bindings::luaBinding<bindings::core::SetSettingValue> },

        luaL_Reg{ "update_gateway", bindings::luaBinding<bindings::core::UpdateGateway> },
        luaL_Reg{ "update_device_status", Core::luaUpdateDeviceStatus },
        luaL_Reg{ "update_device_items_status", bindings::luaBinding<bindings::core::UpdateDeviceItemsStatus> },
        luaL_Reg{ "update_item_value", bindings::luaBinding<bindings::core::UpdateItemValue> },
        luaL_Reg{ "update_item_value_with_min_max", bindings::luaBinding<bindings::core::UpdateItemValueWithMinMax> },
        luaL_Reg{ "update_item_dictionary_value", Core::luaUpdateItemDictionaryValue },
        luaL_Reg{ "update_reachable_state", Core::luaUpdateReachableState },
        luaL_Reg{ "update_setting_status", bindings::luaBinding<bindings::core::UpdateSettingStatus> },
        luaL_Reg{ "update_setting_dictionary_status",
                  bindings::luaBinding<bindings::core::UpdateSettingDictionaryStatus> },
        luaL_Reg{ "update_ready_state", Core::luaUpdateReadyState } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

void Core::setInfo( lua_State* lua, std::unique_ptr<gateways::BaseDeviceInfo>& deviceInfo ) {
    const auto& firmwarePtr = createValueFromField( lua, common::resources::tokens::Firmware );
    const auto& firmware = ::common::iValueCast<::common::ArrayValue>( firmwarePtr.get() );
    if( firmware ) {
        const auto& arr = firmware->get();
        int index = 0;
        for( auto it = arr.begin(); it != arr.end(); it++ ) {
            index++;
            const auto info = ::common::iValueCast<::common::TableValue>( ( *it ).get() );

            if( info ) {
                std::string id = common::iValueCast<common::StringValue>( ( *info )[tokens::Id] )->get();
                std::string version = common::iValueCast<common::StringValue>( ( *info )[tokens::Version] )->get();
                deviceInfo->m_firmware.insert(
                    std::pair<std::string, std::string>( std::to_string( index ) + " " + id, version ) );
            }
        }
    }

    const auto& infoPtr = createValueFromField( lua, common::resources::tokens::Info );
    const auto& info = ::common::iValueCast<::common::TableValue>( infoPtr.get() );

    if( info ) {
        for( auto it = info->get().begin(); it != info->get().end(); it++ ) {
            std::string key = it->first;
            std::string value = common::iValueCast<common::StringValue>( ( *info )[key.c_str()] )->get();
            deviceInfo->m_info.insert( std::pair<std::string, std::string>( key, value ) );
        }
    }
}

int Core::luaAddDevice( lua_State* lua ) {
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if( lua_gettop( lua ) != 1 ) {
        return luaL_error( lua, "expecting exactly 1 arguments" );
    }

    if( lua_istable( lua, -1 ) ) {
        std::string str = getStringField( lua, "gateway_id" );  // TODO check it broken
        char deviceId[gateways::DeviceSettingsManager::MaxNameSize];
        if( snprintf( deviceId,
                      sizeof( deviceId ),
                      "%c%" PRIX32,
                      gateways::toDevicePrefix( gateways::toGatewayID( str.c_str() ) ),
                      hard::common::generateUniqueId() ) > 0 ) {
            std::unique_ptr<gateways::BaseDeviceInfo> devicesInfo = std::make_unique<gateways::BaseDeviceInfo>();
            if( devicesInfo ) {
                devicesInfo->m_gatewayId.assign( str.c_str() );
                devicesInfo->m_name.assign( getStringField( lua, "name" ) );
                devicesInfo->m_type.assign( getStringField( lua, common::resources::lua_events::Type ) );
                devicesInfo->m_parentDeviceId.assign(
                    getStringField( lua, common::resources::lua_events::ParentDeviceId ) );
                devicesInfo->m_deviceTypeId.assign(
                    getStringField( lua, common::resources::lua_events::DeviceTypeId ) );
                devicesInfo->m_category.assign( getStringField( lua, common::resources::tokens::Category ) );
                devicesInfo->m_subCategory.assign( getStringField( lua, common::resources::tokens::SubCategory ) );
                devicesInfo->setBatteryPowered( getBooleanField( lua, common::resources::lua_events::BatteryPowered ) );
                devicesInfo->m_deviceId = deviceId;
                devicesInfo->setPersistent( getBooleanField( lua, "persistent" ) );
                devicesInfo->m_security.assign( getStringField( lua, common::resources::tokens::Security ) );
                devicesInfo->m_status.assign( "idle" );

                setInfo( lua, devicesInfo );

                if( !devicesInfo->m_parentDeviceId.empty() ) {
                    while( 1 ) {
                        const char* deviceId = devicesInfo->m_parentDeviceId.c_str();
                        auto devices =
                            storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
                        if( deviceId && devices ) {
                            std::vector<char> buff;
                            if( !( devices->getBlob( deviceId, buff ) ) ) {
                                MODCOR_LOGE( "Failed to get a current value for the key: %s", deviceId );
                                break;
                            }
                            gateways::BaseDeviceInfo parentDeviceInfo;
                            gateways::ObjectSerializer serializer;
                            if( !serializer.deserialize( deviceId, std::move( buff ), parentDeviceInfo ) ) {
                                MODCOR_LOGE( "Failed to deserialize the key: %s", deviceId );
                                break;
                            }
                            devicesInfo->m_roomId = parentDeviceInfo.getRoomId();
                        }
                        break;
                    }
                }
                else {
                    devicesInfo->m_roomId = getStringField( lua, ::common::resources::tokens::RoomId );
                }
                common::IValue::UPtr readyFlag = createValueFromField( lua, common::resources::tokens::Ready );
                auto command = std::make_unique<lua::responce::UpdateDevice>();
                if( command ) {
                    MODCOR_LOGD( "deviceId: %s", deviceId );
                    command->m_deviceId.assign( deviceId );
                    command->m_key.assign( common::resources::tokens::Ready );
                    command->m_value = std::move( readyFlag );

                    mod.m_exec.send( lua::responce::BUS_NAME, std::move( command ) );
                }

                gateways::ObjectSerializer serializer;
                std::vector<char> vec = serializer.serialize( *devicesInfo );
                auto store = BinaryFileStorageFactory::get().openNamespace( section::DEVICES );
                if( !( store && !vec.empty() && store->setBlob( deviceId, vec ) ) ) {
                    MODCOR_LOGE( "Failed to store data with key: %s", deviceId );
                    return 0;
                }
            }
            lua_settop( lua, 0 );

            auto bus_command = std::make_unique<::lua::responce::AddDevice>();
            if( bus_command && devicesInfo ) {
                bus_command->m_deviceInfo = std::move( devicesInfo );
                mod.m_exec.send(::lua::responce::BUS_NAME, std::move( bus_command ) );
            }

            lua_pushstring( lua, deviceId );
        }
        else {
            lua_settop( lua, 0 );
            lua_pop( lua, 1 );
            lua_pushstring( lua, "" );
        }

        return 1;
    }
    lua_settop( lua, 0 );
    luaL_error( lua, "input parameter of function is not a table" );
    return 0;
}

int Core::luaRemoveDevice( lua_State* lua ) {
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if( lua_gettop( lua ) != 1 ) {
        return luaL_error( lua, "expecting exactly 1 arguments" );
    };

    if( lua_isstring( lua, 1 ) ) {
        const char* deviceId = lua_tolstring( lua, 1, nullptr );

        std::vector<std::string> items;
        fillItemsListByDeviceId( nullptr, items, deviceId );

        bool isServiceNotification = common::isServiceNotification( deviceId );

        {
            auto fav_dev_store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::FAV_DEVS );
            assert( fav_dev_store );
            auto device_store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
            assert( device_store );

            auto childDevices = getChildDevicesByParentDeviceId( deviceId );
            for( const auto& childDevice : childDevices ) {
                if( !device_store->remove( childDevice ) ) {
                    MODCOR_LOGE( "Failed to remove data with key: %s", childDevice.c_str() );
                }
                fav_dev_store->remove( childDevice );
                auto bus_command = std::make_unique<lua::responce::RemoveDevice>();
                if( bus_command ) {
                    bus_command->m_deviceId = childDevice;
                    bus_command->m_isServiceNotification = isServiceNotification;
                    mod.m_exec.send( lua::responce::BUS_NAME, std::move( bus_command ) );
                }

                fillItemsListByDeviceId( nullptr, items, childDevice.c_str() );
            }
            if( !device_store->remove( deviceId ) ) {
                MODCOR_LOGE( "Failed to remove data with key: %s", deviceId );
            }
            fav_dev_store->remove( deviceId );
        }

        {
            auto fav_item_store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::FAV_ITEMS );
            assert( fav_item_store );

            auto item_store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
            assert( item_store );

            for( const auto& item : items ) {
                if( !item_store->remove( item ) ) {
                    MODCOR_LOGE( "Failed to remove item: %s", item.c_str() );
                }
                fav_item_store->remove( item );
            }
        }
        for( const auto& item : items ) {
            auto bus_command = std::make_unique<bus::command::RemoveItem>();
            if( bus_command ) {
                bus_command->m_itemId = item;
                bus_command->m_deviceId = deviceId;
                mod.m_exec.send( "nma", std::move( bus_command ) );
            }
        }

        std::vector<std::string> settings;
        processSettingsByDeviceId( deviceId,
                                   [&settings]( const char* settingId ) { settings.emplace_back( settingId ); } );
        if( !settings.empty() ) {
            auto& setting_store = ::storage::DeviceSettings::get();
            assert( setting_store );
            for( const auto& setting : settings ) {
                setting_store.remove( setting );
                auto bus_command = std::make_unique<settings::responces::SettingRemoved>();
                if( bus_command ) {
                    bus_command->m_settingId = setting;
                    mod.m_exec.send( "nma", std::move( bus_command ) );
                }
            }
        }

        auto bus_command = std::make_unique<lua::responce::RemoveDevice>();
        if( bus_command ) {
            bus_command->m_deviceId = deviceId;
            bus_command->m_isServiceNotification = isServiceNotification;
            mod.m_exec.send( lua::responce::BUS_NAME, std::move( bus_command ) );
        }
    }
    lua_pop( lua, 1 );
    return 0;
}

int Core::luaRemoveItem( lua_State* lua ) {
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if( lua_gettop( lua ) != 1 ) {
        return luaL_error( lua, "expecting exactly 1 arguments" );
    };

    if( lua_isstring( lua, 1 ) ) {
        const char* ItemId = lua_tolstring( lua, 1, nullptr );

        auto fav_item_store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::FAV_ITEMS );
        assert( fav_item_store );
        auto item_store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
        assert( item_store );

        if( !item_store->remove( ItemId ) ) {
            MODCOR_LOGE( "Failed to remove item: %s", ItemId );
        }
        fav_item_store->remove( ItemId );

        auto bus_command = std::make_unique<bus::command::RemoveItem>();
        if( bus_command ) {
            bus_command->m_itemId = ItemId;
            mod.m_exec.send( "nma", std::move( bus_command ) );
        }
    }
    lua_pop( lua, 1 );
    return 0;
}

int Core::luaRemoveItemDictionaryValue( lua_State* lua ) {
    MODCOR_LOGD( "luaRemoveItemDictionaryValue" );
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if( lua_gettop( lua ) != 2 ) {
        return luaL_error( lua, "expecting exactly 2 arguments" );
    };
    std::string item_id;
    if( lua_isstring( lua, 1 ) ) {
        item_id = lua_tolstring( lua, 1, nullptr );
    }
    lua_remove( lua, 1 );
    if( item_id.empty() ) {
        MODCOR_LOGE( "Failed to get user_id_str" );
        lua_pushnil( lua );
        return 0;
    }

    std::string item_dictionary_id;
    if( lua_isstring( lua, 1 ) ) {
        item_dictionary_id = lua_tolstring( lua, 1, nullptr );
    }
    lua_remove( lua, 1 );
    if( item_dictionary_id.empty() ) {
        MODCOR_LOGE( "Failed to get user_id_str" );
        lua_pushnil( lua );
        return 0;
    }

    auto cacheValue = cache::Cache::get().getValueByKey( item_id );
    if( !cacheValue ) {
        MODCOR_LOGW( "Failed to get cacheValue for item_id %s", item_id.c_str() );
        return 0;
    }

    common::IValue::UPtr value = nullptr;
    const auto& currentValuePtr = (common::TableValue*)cacheValue.get();
    if( currentValuePtr ) {
        const auto temp_val = ( *currentValuePtr )[item_dictionary_id];
        if( temp_val ) {
            value = temp_val->clone();
        }
    }

    bool id_exist = common::iValueCast<common::TableValue>( cacheValue.get() )->isKeyExist( item_dictionary_id );
    if( id_exist ) {
        common::iValueCast<common::TableValue>( cacheValue.get() )->remove( item_dictionary_id );
        cache::Cache::get().setValueByKey( item_id, std::move( cacheValue ) );

        auto itemInfo = gateways::ItemInfo::create( item_id );
        if( !itemInfo ) {
            MODCOR_LOGE( "Failed to create item's info" );
            return 0;
        }

        auto bus_command = std::make_unique<lua::responce::UpdateItemDictionary>();
        if( bus_command ) {
            bus_command->m_itemInfo = std::move( itemInfo );
            bus_command->m_operation = common::resources::tokens::Removed;
            bus_command->m_itemDictionaryId.assign( item_dictionary_id );
            bus_command->m_value = std::move( value );
            mod.m_exec.send( lua::responce::BUS_NAME, std::move( bus_command ) );
        }
    }
    return 0;
}

int Core::luaModifyItem( lua_State* lua ) {
    if( lua_gettop( lua ) != 2 ) {
        return luaL_error( lua, "expecting exactly 2 arguments" );
    };

    if( !lua_isstring( lua, 1 ) ) {
        MODCOR_LOGE( "Failed to get id" );
        lua_pushnil( lua );
        return 1;
    }

    const char* item_id = lua_tolstring( lua, 1, nullptr );
    if( item_id == nullptr ) {
        MODCOR_LOGE( "Failed to get item's id" );
        lua_pushnil( lua );
        return 0;
    }
    lua_remove( lua, 1 );

    MODCOR_LOGD( "luaModifyItem id: %s", item_id );
    std::vector<char> buff;
    gateways::ItemInfo itemInfo;
    gateways::ObjectSerializer serializer;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !( store && store->getBlob( item_id, buff ) && serializer.deserialize( std::move( buff ), itemInfo ) ) ) {
        MODCOR_LOGE( "Failed to deserialize key: %s", item_id );
        return 0;
    }
    itemInfo.setId( item_id );

    if( !lua_istable( lua, -1 ) ) {
        lua_settop( lua, 0 );
        luaL_error( lua, "input parameter of function is not a table" );
        return 0;
    }

    itemInfo.setDeviceId( getStringField( lua, common::resources::lua_events::DeviceId ) );
    itemInfo.setName( getStringField( lua, "name" ) );
    itemInfo.setValueType(
        gateways::toItemValueType( getStringField( lua, common::resources::lua_events::ValueType ).c_str() ) );
    itemInfo.setHasGetter( getBooleanField( lua, common::resources::lua_events::HasGetter ) );
    itemInfo.setHasSetter( getBooleanField( lua, common::resources::lua_events::HasSetter ) );
    itemInfo.setVisible( getBooleanField( lua, common::resources::tokens::Show, true ) );
    readTypeSpecificFieldsFromLua( lua, itemInfo );

    itemInfo.setCategory( common::resources::tokens::General );

    if( itemInfo.getValueType() == gateways::ItemValueType::Token ||
        itemInfo.getValueType() == gateways::ItemValueType::Button_state ) {
        auto enumValues = getArrayOfStringField( lua, "enum" );
        itemInfo.setEnumValues( std::move( enumValues ) );
    }
    else if( itemInfo.getValueType() == gateways::ItemValueType::Int ||
             itemInfo.getValueType() == gateways::ItemValueType::Float || itemInfo.isScalable() ) {
        auto newMin = createValueFromField( lua, common::resources::lua_events::ValueMin );
        auto newMax = createValueFromField( lua, common::resources::lua_events::ValueMax );
        if( newMin && newMax ) {
            auto valueMin = itemInfo.castToItemType( std::move( newMin ) );
            auto valueMax = itemInfo.castToItemType( std::move( newMax ) );

            bool isNeedUpdateMinMax = false;
            float valMin = 0, valMax = 0;
            if( valueMin ) {
                common::ScalesConverterVisitor visitor(
                    core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() ),
                    common::ScalesConverterVisitor::Precision );
                valueMin->accept( visitor );
                auto ivalMin = common::iValueCast<common::FloatValue>( std::move( valueMin ) );
                if( ivalMin && ivalMin->get() != valMin ) {
                    valMin = ivalMin->get();
                    isNeedUpdateMinMax = true;
                }
            }

            if( valueMax ) {
                common::ScalesConverterVisitor visitor(
                    core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() ),
                    common::ScalesConverterVisitor::Precision );
                valueMax->accept( visitor );
                auto ivalMax = common::iValueCast<common::FloatValue>( std::move( valueMax ) );
                if( ivalMax && ivalMax->get() != valMax ) {
                    valMax = ivalMax->get();
                    isNeedUpdateMinMax = true;
                }
            }

            if( isNeedUpdateMinMax ) {
                itemInfo.setMinMax( { valMin, valMax } );
            }
        }
    }

    std::vector<char> vec = serializer.serialize( itemInfo );
    if( !( store && !vec.empty() && store->setBlob( itemInfo.getId(), std::move( vec ) ) ) ) {
        lua_settop( lua, 0 );
        MODCOR_LOGE( "Can't save ItemInfo to storage for %s", itemInfo.getId().c_str() );
        return 0;
    }

    auto value = itemInfo.castToItemType( createValueFromField( lua, "value" ) );

    lua_settop( lua, 0 );

    if( value ) {
        value = itemInfo.castToItemType( std::move( value ) );
    }

    if( value ) {
        cache::Cache::get().setValueByKey( itemInfo.getId(), std::move( value ) );
    }
    else {
        MODCOR_LOGI( "Item's value is absent" );
    }
    return 1;
}

int Core::luaGetDevice( lua_State* lua ) {
    if( lua_gettop( lua ) != 1 ) {
        return luaL_error( lua, "expecting exactly 1 arguments" );
    };

    if( !lua_isstring( lua, 1 ) ) {
        MODCOR_LOGE( "Failed to get id" );
        lua_pushnil( lua );
        return 1;
    }

    const char* keyValue = lua_tolstring( lua, 1, nullptr );
    if( !keyValue ) {
        MODCOR_LOGE( "Failed to get device's id" );
        lua_pushnil( lua );
        return 0;
    }

    std::vector<char> buff;
    gateways::BaseDeviceInfo deviceInfo;
    gateways::ObjectSerializer serializer;
    auto store = BinaryFileStorageFactory::get().openNamespace( section::DEVICES );
    if( !( store && store->getBlob( keyValue, buff ) &&
           serializer.deserialize( keyValue, std::move( buff ), deviceInfo ) ) ) {
        MODCOR_LOGE( "Failed to deserialize key: %s", keyValue );
        return 0;
    }

    pushDeviceToLua( lua, deviceInfo );
    return 1;
}

int Core::luaGetDevices( lua_State* lua ) {
    auto store = BinaryFileStorageFactory::get().openNamespace( section::DEVICES );
    if( !store ) {
        MODCOR_LOGE( "Failed to open store" );
        return 0;
    }

    bool result{ false };

    lua_createtable( lua, 0, 1 );  // TODO  better to use number of devices
    int index = 1;
    auto it = store->cbegin(), end = store->cend();
    for( ; it != end; ++it ) {
        auto val = it.value();
        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ) {
            MODCOR_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
            continue;
        }
        gateways::BaseDeviceInfo deviceInfo;
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( *it, std::move( val.get<BlobType>() ), deviceInfo ) ) {
            MODCOR_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );
            continue;
        }
        result = true;

        lua_pushinteger( lua, index++ );

        pushDeviceToLua( lua, deviceInfo );

        lua_settable( lua, -3 );
    }
    return result ? 1 : 0;
}

int Core::luaModifyDevice( lua_State* lua ) {
    if( lua_gettop( lua ) != 2 ) {
        return luaL_error( lua, "expecting exactly 2 arguments" );
    };

    if( !lua_isstring( lua, 1 ) ) {
        MODCOR_LOGE( "Failed to get id" );
        lua_pushnil( lua );
        return 1;
    }

    const char* device_id = lua_tolstring( lua, 1, nullptr );
    if( !device_id ) {
        MODCOR_LOGE( "Failed to get device's id" );
        lua_pushnil( lua );
        return 0;
    }

    MODCOR_LOGD( "luaModifyDevice id: %s", device_id );
    std::vector<char> buff;
    std::unique_ptr<gateways::BaseDeviceInfo> deviceInfo = std::make_unique<gateways::BaseDeviceInfo>();
    gateways::ObjectSerializer serializer;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !( store && store->getBlob( device_id, buff ) &&
           serializer.deserialize( device_id, std::move( buff ), *deviceInfo ) ) ) {
        MODCOR_LOGE( "Failed to deserialize key: %s", device_id );
        return 0;
    }

    deviceInfo->m_type.assign( getStringField( lua, common::resources::lua_events::Type ) );
    deviceInfo->m_category.assign( getStringField( lua, common::resources::tokens::Category ) );
    deviceInfo->m_subCategory.assign( getStringField( lua, common::resources::tokens::SubCategory ) );
    deviceInfo->m_security.assign( getStringField( lua, common::resources::tokens::Security ) );
    deviceInfo->m_name.assign( getStringField( lua, common::resources::tokens::Name ) );
    deviceInfo->m_parentDeviceId.assign( getStringField( lua, common::resources::lua_events::ParentDeviceId ) );
    setInfo( lua, deviceInfo );

    std::vector<char> vec = serializer.serialize( *deviceInfo );
    if( !( store && !vec.empty() && store->setBlob( device_id, vec ) ) ) {
        MODCOR_LOGE( "Failed to store data with key: %s", device_id );
        return 0;
    }

    return 1;
}

Executor& Core::executorFromLua(::lua_State* lua ) {
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    return mod.getExecutor();
}

void Core::sendReachableState( Executor& executor, const char* device_id, const bool reachable ) {
    std::string key = std::string( device_id ) + common::resources::tokens::Reachable;
    auto current_value = common::iValueCast<common::BoolValue>( cache::Cache::get().getValueByKey( key ) );
    auto current_state = current_value ? current_value->get() : false;
    if( current_state != reachable ) {
        cache::Cache::get().setValueByKey( key, std::move( std::make_unique<common::BoolValue>( reachable ) ) );
        auto command = std::make_unique<lua::responce::UpdateDevice>();
        if( command ) {
            command->m_deviceId.assign( device_id );
            command->m_key.assign( common::resources::tokens::Reachable );
            command->m_value = std::move( std::make_unique<common::BoolValue>( reachable ) );

            executor.send( lua::responce::BUS_NAME, std::move( command ) );
        }
    }
}

int Core::luaUpdateDeviceStatus( lua_State* lua ) {
    if( lua_gettop( lua ) != 2 ) {
        return luaL_error( lua, "expecting exactly 2 arguments" );
    }
    if( !lua_isstring( lua, 1 ) ) {
        MODCOR_LOGE( "Failed to get id" );
        lua_pushnil( lua );
        return 1;
    }

    const char* device_id = lua_tolstring( lua, 1, nullptr );
    if( device_id == nullptr ) {
        MODCOR_LOGE( "Failed to get device's id" );
        lua_pushnil( lua );
        return 0;
    }

    if( !lua_isstring( lua, 1 ) ) {
        MODCOR_LOGE( "Failed to get status" );
        lua_pushnil( lua );
        return 1;
    }

    const char* status = lua_tolstring( lua, 2, nullptr );
    if( status == nullptr ) {
        MODCOR_LOGE( "Failed to get status for device id %s", device_id );
        lua_pushnil( lua );
        return 0;
    }

    auto deviceInfo = gateways::BaseDeviceInfo::create( device_id );
    if( !deviceInfo ) {
        return 0;
    }

    deviceInfo->setStatus( status );

    gateways::ObjectSerializer serializer;
    std::vector<char> vec = serializer.serialize( *deviceInfo );
    auto store = BinaryFileStorageFactory::get().openNamespace( section::DEVICES );
    if( !( store && !vec.empty() && store->setBlob( device_id, vec ) ) ) {
        MODCOR_LOGE( "Failed to store data with key: %s", device_id );
        return 0;
    }

    return 0;
}

int Core::luaUpdateReachableState( lua_State* lua ) {
    if( lua_gettop( lua ) != 2 ) {
        return luaL_error( lua, "expecting exactly 2 arguments" );
    }
    if( !lua_isstring( lua, 1 ) ) {
        MODCOR_LOGE( "Failed to get device's id" );
        lua_pushnil( lua );
        return 0;
    }
    const char* deviceId = lua_tolstring( lua, 1, nullptr );
    if( deviceId == nullptr ) {
        MODCOR_LOGE( "Failed to get device's id" );
        lua_pushnil( lua );
        return 0;
    }

    if( !lua_isboolean( lua, 2 ) ) {
        MODCOR_LOGE( "Failed to get value" );
        lua_pushnil( lua );
        return 0;
    }
    const bool value = lua_toboolean( lua, 2 );
    sendReachableState( executorFromLua( lua ), deviceId, value );
    return 0;
}

int Core::luaUpdateReadyState( lua_State* lua ) {
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if( lua_gettop( lua ) != 2 ) {
        return luaL_error( lua, "expecting exactly 2 arguments" );
    }
    if( !lua_isstring( lua, 1 ) ) {
        MODCOR_LOGE( "Failed to get device's id" );
        lua_pushnil( lua );
        return 0;
    }
    const char* deviceId = lua_tolstring( lua, 1, nullptr );
    if( deviceId == nullptr ) {
        MODCOR_LOGE( "Failed to get device's id" );
        lua_pushnil( lua );
        return 0;
    }

    if( !lua_isboolean( lua, 2 ) ) {
        MODCOR_LOGE( "Failed to get value" );
        lua_pushnil( lua );
        return 0;
    }
    const bool value = lua_toboolean( lua, 2 );
    auto command = std::make_unique<lua::responce::UpdateDevice>();
    if( command ) {
        command->m_deviceId.assign( deviceId );
        command->m_key.assign( common::resources::tokens::Ready );
        command->m_value = std::move( std::make_unique<common::BoolValue>( value ) );
        mod.m_exec.send( lua::responce::BUS_NAME, std::move( command ) );
    }
    return 0;
}

int Core::luaSendUiBroadcast( lua_State* lua ) {
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if( lua_gettop( lua ) != 1 ) {
        return luaL_error( lua, "expecting exactly 1 arguments" );
    }

    auto command = std::make_unique<::lua::responce::HubSendBroadcast>();
    if( command ) {
        command->m_event.assign( getStringField( lua, "event" ) );
        command->m_plugin.assign( getPluginId( lua ) );

        if( strcmp( command->m_event.c_str(), common::resources::broadcasts::events::S2SelectAuthenticationModes ) ==
            0 ) {
            lua_pushstring( lua, "modes" );
            lua_gettable( lua, -2 );
            if( lua_istable( lua, -1 ) ) {
                lua_pushvalue( lua, -1 );
                size_t len = lua_rawlen( lua, -1 );
                for( int i = 1; len >= i; ++i ) {
                    lua_pushnumber( lua, i );
                    lua_gettable( lua, -2 );

                    std::string result;
                    result.assign( lua_tostring( lua, -1 ) );
                    command->m_value.push_back( result );

                    lua_pop( lua, 1 );  // stack: vector
                }
                lua_pop( lua, 1 );
            }
            lua_pop( lua, 1 );  // remove string from stack
        }
        else if( strcmp( command->m_event.c_str(),
                         common::resources::broadcasts::events::S2RequestDeviceSpecificKey ) == 0 ||
                 strcmp( command->m_event.c_str(), common::resources::broadcasts::events::S2ShowDeviceSideKey ) == 0 ||
                 strcmp( command->m_event.c_str(), common::resources::broadcasts::events::LearnShowKey ) == 0 ) {
            lua_pushstring( lua, "key" );
            lua_gettable( lua, -2 );
            if( lua_istable( lua, -1 ) ) {
                lua_pushvalue( lua, -1 );
                size_t len = lua_rawlen( lua, -1 );
                for( int i = 1; len >= i; ++i ) {
                    lua_pushnumber( lua, i );
                    lua_gettable( lua, -2 );

                    std::string result;
                    result.assign( lua_tostring( lua, -1 ) );
                    command->m_value.push_back( result );

                    lua_pop( lua, 1 );  // stack: vector
                }
                lua_pop( lua, 1 );
            }
            lua_pop( lua, 1 );  // remove string from stack
        }
        else if( strcmp( command->m_event.c_str(), common::resources::broadcasts::events::IncludeDeviceProgress ) ==
                 0 ) {
            char progress[4];
            sprintf( progress, "%u", getIntegerField( lua, common::resources::lua_events::Value ) );
            command->m_value.push_back( progress );
        }
        else if( strcmp( command->m_event.c_str(), common::resources::broadcasts::events::LearnProgress ) == 0 ||
                 strcmp( command->m_event.c_str(), common::resources::broadcasts::events::ReconstructProgress ) == 0 ) {
            char temp[4];
            sprintf( temp, "%u", getIntegerField( lua, common::resources::lua_events::Devices ) );
            command->m_value.push_back( temp );
            sprintf( temp, "%u", getIntegerField( lua, common::resources::lua_events::Current ) );
            command->m_value.push_back( temp );
        }
        else if( strcmp( command->m_event.c_str(), common::resources::broadcasts::events::ReconstructProgressError ) ==
                 0 ) {
            char temp[4];
            sprintf( temp, "%u", getIntegerField( lua, common::resources::lua_events::NodeId ) );
            command->m_value.push_back( temp );
        }
        else if( strcmp( command->m_event.c_str(), common::resources::broadcasts::events::RediscoverDeviceProgress ) ==
                 0 ) {
            char temp[4];
            sprintf( temp, "%u", getIntegerField( lua, common::resources::lua_events::Progress ) );
            command->m_value.push_back( temp );
            command->m_value.push_back( getStringField( lua, common::resources::lua_events::Status ) );
        }
        else if( strcmp( command->m_event.c_str(), common::resources::broadcasts::events::ModuleReset ) == 0 ) {
            command->m_value.push_back( getStringField( lua, common::resources::lua_events::Status ) );
        }
        mod.m_exec.send(::lua::responce::BUS_NAME, std::move( command ) );
    }

    lua_settop( lua, 0 );
    return 0;
}

int Core::luaGetProductName( lua_State* lua ) {
    auto pname = core::IDInfo::get().productName();
    if( pname ) {
        lua_pushlstring( lua, pname, std::strlen( pname ) );
    }
    else {
        char empty[] = "";
        lua_pushlstring( lua, empty, 0 );
    }

    return 1;
}

int Core::luaGenerateItemDictionaryNumberId( lua_State* lua ) {
    MODCOR_LOGD( "Core::luaGenerateItemDictionaryNumberId" );
    if( lua_gettop( lua ) != 3 ) {
        return luaL_error( lua, "expecting exactly 3 arguments" );
    };

    std::string item_id;
    if( lua_isstring( lua, 1 ) ) {
        item_id = lua_tolstring( lua, 1, nullptr );
    }
    lua_remove( lua, 1 );
    if( item_id.empty() ) {
        MODCOR_LOGE( "Failed to get user_id_str" );
        lua_pushnil( lua );
        return 0;
    }
    const uint8_t min_id = lua_tointeger( lua, 1 );
    lua_remove( lua, 1 );
    const uint8_t max_id = lua_tointeger( lua, 1 );
    lua_remove( lua, 1 );

    MODCOR_LOGD( "min_id  %u, max_id %u", min_id, max_id );

    auto cacheValue = cache::Cache::get().getValueByKey( item_id );
    if( !cacheValue ) {
        lua_settop( lua, 1 );
        lua_pushinteger( lua, min_id );
        return 1;
    }

    uint8_t i = max_id + 1;
    char item_dictionary_id[4];
    if( cacheValue ) {
        for( i = min_id; i <= max_id; ++i ) {
            sprintf( item_dictionary_id, "%u", i );
            if( !( common::iValueCast<common::TableValue>( cacheValue.get() )->isKeyExist( item_dictionary_id ) ) )
                break;
        }
    }

    MODCOR_LOGD( "%u", i );

    lua_settop( lua, 1 );
    if( i > max_id ) {
        lua_pushnil( lua );
        return 0;
    }
    else {
        lua_pushinteger( lua, i );
        return 1;
    }
}

int Core::luaAddItem( lua_State* lua ) {
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if( lua_gettop( lua ) != 1 ) {
        return luaL_error( lua, "expecting exactly 1 arguments" );
    }

    if( !lua_istable( lua, -1 ) ) {
        lua_settop( lua, 0 );
        luaL_error( lua, "input parameter of function is not a table" );
        return 0;
    }

    std::unique_ptr<gateways::ItemInfo> itemInfo;
    {
        char itemId[gateways::DeviceSettingsManager::MaxNameSize];
        std::string name = std::move( getStringField( lua, "name" ) );
        MODCOR_LOGD( "item_id=%s", name.c_str() );
        if( snprintf( itemId, sizeof( itemId ), "%X", hard::common::generateUniqueId() ) <= 0 ) {
            lua_settop( lua, 0 );
            MODCOR_LOGE( "Can't create itemId" );
            return 0;
        }

        itemInfo = std::make_unique<gateways::ItemInfo>(
            getStringField( lua, common::resources::lua_events::DeviceId ),
            std::move( name ),
            std::string( itemId ),
            gateways::toItemValueType( getStringField( lua, common::resources::lua_events::ValueType ).c_str() ),
            getBooleanField( lua, common::resources::lua_events::HasGetter ),
            getBooleanField( lua, common::resources::lua_events::HasSetter ),
            getBooleanField( lua, common::resources::tokens::Show, true ),
            getStringField( lua, common::resources::lua_events::UserCodeRestriction ),
            common::resources::tokens::General,  // TODO replace this line by "( getStringField( lua,
                                                 // common::resources::tokens::Category ).c_str() )" when item
                                                 // 'Category' will be available in LUA
            gateways::ItemInfo::stringToStatus( getStringField( lua, common::resources::tokens::Status ).c_str() ) );

        itemInfo->setElementsMaxNumber( getIntegerField( lua, common::resources::lua_events::ElementsMaxNumber ) );
        readTypeSpecificFieldsFromLua( lua, *itemInfo );
    }

    if( auto statusString = ::common::iValueCast<::common::StringValue>(
            createValueFromField( lua, ::common::resources::lua_events::Status ) ) ) {
        itemInfo->setStatus( statusString->get() );
    }
    else {
        itemInfo->setStatus(::common::resources::lua_events::Outdated );
    }

    auto value = itemInfo->castToItemType( createValueFromField( lua, "value" ) );
    if( value ) {
        common::ScalesConverterVisitor visitor(
            core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() ),
            common::ScalesConverterVisitor::Precision );
        value->accept( visitor );
        cache::Cache::get().setValueByKey( itemInfo->getId(), value->clone() );
    }

    if( itemInfo->getValueType() == gateways::ItemValueType::Token ||
        itemInfo->getValueType() == gateways::ItemValueType::Button_state ) {
        auto enumValues = getArrayOfStringField( lua, "enum" );
        itemInfo->setEnumValues( std::move( enumValues ) );
    }
    else if( itemInfo->getValueType() == gateways::ItemValueType::Int ||
             itemInfo->getValueType() == gateways::ItemValueType::Float || itemInfo->isScalable() ) {
        auto valueMin =
            itemInfo->castToItemType( createValueFromField( lua, common::resources::lua_events::ValueMin ) );
        auto valueMax =
            itemInfo->castToItemType( createValueFromField( lua, common::resources::lua_events::ValueMax ) );

        bool isNeedUpdateMinMax = false;
        float valMin = 0, valMax = 0;
        if( valueMin ) {
            common::ScalesConverterVisitor visitor(
                core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() ),
                common::ScalesConverterVisitor::Precision );
            valueMin->accept( visitor );
            auto ivalMin = common::iValueCast<common::FloatValue>( std::move( valueMin ) );
            if( ivalMin && ivalMin->get() != valMin ) {
                valMin = ivalMin->get();
                isNeedUpdateMinMax = true;
            }
        }

        if( valueMax ) {
            common::ScalesConverterVisitor visitor(
                core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() ),
                common::ScalesConverterVisitor::Precision );
            valueMax->accept( visitor );
            auto ivalMax = common::iValueCast<common::FloatValue>( std::move( valueMax ) );
            if( ivalMax && ivalMax->get() != valMax ) {
                valMax = ivalMax->get();
                isNeedUpdateMinMax = true;
            }
        }

        if( isNeedUpdateMinMax ) {
            itemInfo->setMinMax( { valMin, valMax } );
        }
    }

    {
        gateways::ObjectSerializer serializer;
        std::vector<char> vec = serializer.serialize( *itemInfo );
        auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
        if( !( store && !vec.empty() && store->setBlob( itemInfo->getId(), std::move( vec ) ) ) ) {
            lua_settop( lua, 0 );
            MODCOR_LOGE( "Can't save ItemInfo to storage for %s", itemInfo->getId().c_str() );
            return 0;
        }
    }

    lua_settop( lua, 0 );
    lua_pushstring( lua, itemInfo->getId().c_str() );

    auto bus_command = std::make_unique<lua::responce::AddItem>();
    if( bus_command && itemInfo ) {
        bus_command->m_itemInfo = std::move( itemInfo );
        mod.m_exec.send( lua::responce::BUS_NAME, std::move( bus_command ) );
    }

    return 1;
}

int Core::luaGetItem( lua_State* lua ) {
    if( lua_gettop( lua ) != 1 ) {
        return luaL_error( lua, "expecting exactly 1 arguments" );
    };

    if( !lua_isstring( lua, 1 ) ) {
        MODCOR_LOGE( "Failed to get id" );
        lua_pushnil( lua );
        return 0;
    }

    const char* keyValue = lua_tolstring( lua, 1, nullptr );
    if( keyValue == nullptr ) {
        MODCOR_LOGE( "Failed to get id" );
        lua_pushnil( lua );
        return 0;
    }

    std::vector<char> buff;
    gateways::ItemInfo itemInfo;
    gateways::ObjectSerializer serializer;
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !( store && store->getBlob( keyValue, buff ) && serializer.deserialize( std::move( buff ), itemInfo ) ) ) {
        MODCOR_LOGE( "Failed to deserialize key: %s", keyValue );
        return 0;
    }
    itemInfo.setId( keyValue );

    pushItemToLua( lua, itemInfo );
    return 1;
}

int Core::luaGetItems( lua_State* lua ) {
    std::vector<std::string> items;
    fillItemsListByDeviceId( lua, items );

    return items.empty() ? 0 : 1;
}

int Core::luaGetItemsByDeviceId( lua_State* lua ) {
    if( lua_gettop( lua ) != 1 || !lua_isstring( lua, 1 ) ) {
        return luaL_error( lua, "expecting exactly 1 string argument" );
    };
    const char* deviceId = lua_tostring( lua, 1 );
    std::vector<std::string> items;
    fillItemsListByDeviceId( lua, items, deviceId );
    return items.empty() ? 0 : 1;
}

int Core::luaUpdateItemDictionaryValue( lua_State* lua ) {
    MODCOR_LOGD( "luaUpdateItemDictionaryValue" );
    Core& mod = *( (Core*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if(::lua_gettop( lua ) < 3 ) {
        return luaL_error( lua, "expecting at least 3 arguments" );
    };

    std::string item_id;
    if( lua_isstring( lua, 1 ) ) {
        item_id = lua_tolstring( lua, 1, nullptr );
    }
    lua_remove( lua, 1 );
    if( item_id.empty() ) {
        MODCOR_LOGE( "Failed to get user_id_str" );
        lua_pushnil( lua );
        return 0;
    }

    std::string item_dictionary_id;
    if( lua_isstring( lua, 1 ) ) {
        item_dictionary_id = lua_tolstring( lua, 1, nullptr );
    }
    lua_remove( lua, 1 );
    if( item_dictionary_id.empty() ) {
        MODCOR_LOGE( "Failed to get item_dictionary_id" );
        lua_pushnil( lua );
        return 0;
    }

    auto newValue = createValueFromStack( lua );
    lua_remove( lua, 1 );

    auto itemInfo = gateways::ItemInfo::create( item_id );
    if( !itemInfo ) {
        MODCOR_LOGE( "Failed to create item's info" );
        return 0;
    }
    gateways::ItemInfo& refItemInfo = *( itemInfo.get() );
    sendReachableState( executorFromLua( lua ), refItemInfo.getDeviceId().c_str(), true );
    itemInfo->setStatus(::lua_gettop( lua ) ? ::lua::module::converters::loadValue<std::string>( lua ) :
                                              ::common::resources::lua_events::Synced );

    bool id_exist = false;
    auto cacheValue = cache::Cache::get().getValueByKey( item_id );
    if( !cacheValue ) {
        auto defaultValue = refItemInfo.createDefaultValue();
        common::iValueCast<common::TableValue>( defaultValue.get() )->insert( item_dictionary_id, newValue->clone() );
        cache::Cache::get().setValueByKey( item_id, std::move( defaultValue ) );
    }
    else {
        auto table = common::iValueCast<common::TableValue>( cacheValue.get() );
        if( table ) {
            id_exist = table->isKeyExist( item_dictionary_id );
            common::iValueCast<common::TableValue>( cacheValue.get() )->insert( item_dictionary_id, newValue->clone() );
            cache::Cache::get().setValueByKey( item_id, std::move( cacheValue ) );
        }
        else {
            auto defaultValue = refItemInfo.createDefaultValue();
            common::iValueCast<common::TableValue>( defaultValue.get() )
                ->insert( item_dictionary_id, newValue->clone() );
            cache::Cache::get().setValueByKey( item_id, std::move( defaultValue ) );
        }
    }

    auto bus_command = std::make_unique<lua::responce::UpdateItemDictionary>();
    if( bus_command ) {
        bus_command->m_itemInfo = std::move( itemInfo );
        bus_command->m_operation = id_exist ? common::resources::tokens::Updated : common::resources::tokens::Added;
        bus_command->m_itemDictionaryId.assign( item_dictionary_id );
        bus_command->m_value = std::move( newValue );
        mod.m_exec.send( lua::responce::BUS_NAME, std::move( bus_command ) );
    }
    return 0;
}

int Core::luaRemoveGatewayDevices( lua_State* lua ) {
    if( lua_gettop( lua ) != 1 ) {
        return luaL_error( lua, "expecting exactly 1 arguments" );
    };

    if( lua_isstring( lua, 1 ) ) {
        const char* gatewayId = lua_tolstring( lua, 1, nullptr );

        std::vector<std::string> devices;
        std::vector<std::string> items;
        processDevicesByGatewayId( gatewayId, [&devices, &items]( const char* deviceId ) {
            processItemsByDeviceId( deviceId, [&items]( const char* itemId ) { items.emplace_back( itemId ); } );
            devices.emplace_back( deviceId );
        } );

        {
            auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
            if( !store ) {
                MODCOR_LOGE( "Failed to open store" );
                return 0;
            }
            for( const auto& key : items ) {
                if( !( store->remove( key ) ) ) {
                    MODCOR_LOGE( "Failed to remove a current value for the key: %s", key.c_str() );
                    continue;
                }
            }
        }

        {
            auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
            if( !store ) {
                MODCOR_LOGE( "Failed to open store" );
                return 0;
            }
            for( const auto& key : devices ) {
                if( !( store->remove( key ) ) ) {
                    MODCOR_LOGE( "Failed to remove a current value for the key: %s", key.c_str() );
                    continue;
                }
            }
        }
    }
    return 0;
}

void Core::processDevicesByGatewayId( const char* gatewayId, const std::function<void( const char* deviceId )>& func ) {
    auto store = BinaryFileStorageFactory::get().openNamespace( section::DEVICES );
    if( !store ) {
        MODCOR_LOGE( "Failed to open store" );
        return;
    }

    if( gatewayId == nullptr ) {
        return;
    }

    auto it = store->cbegin(), end = store->cend();
    for( ; it != end; ++it ) {
        auto val = it.value();
        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ) {
            MODCOR_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
            continue;
        }
        gateways::BaseDeviceInfo deviceInfo;
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( *it, std::move( val.get<BlobType>() ), deviceInfo ) ) {
            MODCOR_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );
            continue;
        }

        if( deviceInfo.m_gatewayId != gatewayId ) {
            continue;
        }

        func( deviceInfo.getId() );
    }
}

void Core::processItemsByDeviceId( const char* deviceId, const std::function<void( const char* itemId )>& func ) {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !store ) {
        MODCOR_LOGE( "Failed to open store" );
        return;
    }

    if( deviceId == nullptr ) {
        return;
    }

    auto it = store->cbegin(), end = store->cend();
    for( ; it != end; ++it ) {
        auto val = it.value();
        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ) {
            MODCOR_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
            continue;
        }
        gateways::ItemInfo itemInfo;
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( std::move( val.get<BlobType>() ), itemInfo ) ) {
            MODCOR_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );
            continue;
        }

        if( itemInfo.getDeviceId() != deviceId ) {
            continue;
        }

        itemInfo.setId( *it );
        func( itemInfo.getId().c_str() );
    }
}

void Core::processSettingsByDeviceId( const char* deviceId, const std::function<void( const char* settingId )>& func ) {
    ::storage::DeviceSettings::get().enumerate( [&]( const gateways::DeviceSettingsInfo& info ) {
        if( info.getItemId() == deviceId ) {
            func( info.getId().c_str() );
        }
        return true;
    } );
}

void Core::fillItemsListByDeviceId( lua_State* lua, std::vector<std::string>& items, const char* deviceId ) {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !store ) {
        MODCOR_LOGE( "Failed to open store" );
        return;
    }

    if( lua ) {
        lua_newtable( lua );
    }

    int index = 1;

    //  TODO use processItemsByDeviceId ---->
    auto it = store->cbegin(), end = store->cend();
    for( ; it != end; ++it ) {
        auto val = it.value();
        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ) {
            MODCOR_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
            continue;
        }
        gateways::ItemInfo itemInfo;
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( std::move( val.get<BlobType>() ), itemInfo ) ) {
            MODCOR_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );
            continue;
        }

        if( deviceId && itemInfo.getDeviceId() != deviceId ) {
            continue;
        }

        items.emplace_back( *it );
        itemInfo.setId( *it );

        if( lua ) {
            lua_pushinteger( lua, index++ );
            pushItemToLua( lua, itemInfo );
            lua_settable( lua, -3 );
        }
    }

    items.shrink_to_fit();
    return;
}

std::vector<std::string> Core::getChildDevicesByParentDeviceId( const char* deviceId ) {
    std::vector<std::string> devices;

    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !store ) {
        MODCOR_LOGE( "Failed to open store" );
        return {};
    }

    //  TODO use processItemsByDeviceId ---->
    auto it = store->cbegin(), end = store->cend();
    for( ; it != end; ++it ) {
        auto val = it.value();
        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ) {
            MODCOR_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
            continue;
        }
        gateways::BaseDeviceInfo deviceInfo;
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( *it, std::move( val.get<BlobType>() ), deviceInfo ) ) {
            MODCOR_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );
            continue;
        }

        if( deviceId && strcmp( deviceInfo.getParentDeviceId(), deviceId ) != 0 ) {
            continue;
        }

        devices.emplace_back( *it );
    }

    devices.shrink_to_fit();
    return devices;
}

void Core::pushDeviceToLua( lua_State* lua, const gateways::BaseDeviceInfo& deviceInfo ) {
    lua_createtable( lua, 0, 1 );

    lua_pushstring( lua, deviceInfo.getId() );
    lua_setfield( lua, -2, common::resources::tokens::Id );

    lua_pushstring( lua, deviceInfo.getParentDeviceId() );
    lua_setfield( lua, -2, common::resources::lua_events::ParentDeviceId );

    lua_pushstring( lua, deviceInfo.getGatewayId() );
    lua_setfield( lua, -2, common::resources::lua_events::GatewayId );

    lua_pushstring( lua, deviceInfo.getName() );
    lua_setfield( lua, -2, common::resources::tokens::Name );

    lua_pushstring( lua, deviceInfo.getCategory() );
    lua_setfield( lua, -2, common::resources::tokens::Category );

    lua_pushstring( lua, deviceInfo.getSubcategory() );
    lua_setfield( lua, -2, common::resources::tokens::SubCategory );

    lua_pushstring( lua, deviceInfo.getType() );
    lua_setfield( lua, -2, common::resources::lua_events::ValueType );

    lua_pushstring( lua, deviceInfo.getDeviceTypeId() );
    lua_setfield( lua, -2, common::resources::lua_events::DeviceTypeId );

    lua_pushstring( lua, deviceInfo.getRoomId() );
    lua_setfield( lua, -2, ::common::resources::tokens::RoomId );

    lua_pushstring( lua, deviceInfo.getSecurity() );
    lua_setfield( lua, -2, common::resources::tokens::Security );

    lua_pushboolean( lua, deviceInfo.isBatteryPowered() );
    lua_setfield( lua, -2, common::resources::lua_events::BatteryPowered );

    lua_pushboolean( lua, deviceInfo.isReachable() );
    lua_setfield( lua, -2, common::resources::tokens::Reachable );

    lua_pushboolean( lua, deviceInfo.isPersistent() );
    lua_setfield( lua, -2, common::resources::tokens::Persistent );

    lua_pushboolean( lua, deviceInfo.isReady() );
    lua_setfield( lua, -2, common::resources::tokens::Ready );

    lua_pushstring( lua, deviceInfo.getStatus() );
    lua_setfield( lua, -2, common::resources::tokens::Status );
}

void Core::pushItemToLua( lua_State* lua, const gateways::ItemInfo& itemInfo ) {
    lua_createtable( lua, 0, 1 );

    lua_pushstring( lua, itemInfo.getId().c_str() );
    lua_setfield( lua, -2, common::resources::tokens::Id );

    lua_pushstring( lua, itemInfo.getDeviceId().c_str() );
    lua_setfield( lua, -2, common::resources::lua_events::DeviceId );

    lua_pushstring( lua, itemInfo.getName().c_str() );
    lua_setfield( lua, -2, common::resources::tokens::Name );

    lua_pushstring( lua, toString( itemInfo.getValueType() ) );
    lua_setfield( lua, -2, common::resources::lua_events::ValueType );

    lua_pushboolean( lua, itemInfo.hasGetter() );
    lua_setfield( lua, -2, common::resources::lua_events::HasGetter );

    lua_pushboolean( lua, itemInfo.hasSetter() );
    lua_setfield( lua, -2, common::resources::lua_events::HasSetter );

    if( itemInfo.isEnum() ) {
        lua_newtable( lua );
        for( size_t i = 0; i < itemInfo.getEnumItemsCount(); ++i ) {
            ::lua_pushinteger( lua, i + 1 );
            ::lua_pushstring( lua, itemInfo.getEnumValue( i ).c_str() );
            ::lua_settable( lua, -3 );
        }
        lua_setfield( lua, -2, common::resources::tokens::Enum );
    }

    for( auto& field : itemInfo.getTypeSpecificFields() ) {
        ValueToLuaGenerator gen{ lua };
        field.second->accept( gen );
        ::lua_setfield( lua, -2, field.first.c_str() );
    }

    if( itemInfo.getId().empty() ) {
        return;
    }
    auto cacheValue = cache::Cache::get().getValueByKey( itemInfo.getId() );
    if( cacheValue ) {
        ValueToLuaGenerator generator( lua );
        cacheValue->accept( generator );
        lua_setfield( lua, -2, common::resources::tokens::Value );
    }
    else {
        lua_newtable( lua );
        lua_setfield( lua, -2, common::resources::tokens::Value );
    }
}

void Core::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::HandleEventUpdateProductName& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        MODCOR_LOGE( "The lua's state is empty" );
        return;
    }

    if( !command.event().empty() ) {
        lua_createtable( lua, 0, 1 );

        lua_pushstring( lua, command.event().c_str() );
        lua_setfield( lua, -2, "event" );

        m_exec.luaRet( 1 );
    }
    else {
        MODCOR_LOGE( "HandleEvent. An event's name is empty" );
    }
}

void Core::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::HubSettingsApply& command ) {
    bool setValueResult = false;
    const auto& name = command.getName();
    if( !name.empty() && command.getValue() ) {
        if( name == tokens::BroadcastsDelay ) {
            auto value = common::iValueCast<common::TableValue>( command.getValue() );
            if( value ) {
                setValueResult = true;
                const auto end = value->cend();
                for( auto itr = value->cbegin(); end != itr; ++itr ) {
                    auto second = common::iValueCast<common::IntValue>( itr->second.get() );
                    if( itr->first.empty() || !second ) {
                        continue;
                    }
                    setValueResult = setValueResult && core::HubSettings::get().setBroadcastDelayForCategory(
                                                           itr->first.c_str(), second->get() );
                }
            }
        }
        else if( name == tokens::ScaleTemperature ) {
            auto value = common::iValueCast<common::StringValue>( command.getValue()->clone() );
            if( value && !value->get().empty() ) {
                setValueResult = true;
                setValueResult = setValueResult && hubSettingsApplyTemperatureScale( value->get() );
                setValueResult = setValueResult && hubSettingsApplyTemperatureScaleSettings( value->get() );
                setValueResult =
                    setValueResult && core::HubSettings::get().setTemperatureScale(
                                          core::HubSettingsInfo::strToTemperatureScale( value->get().c_str() ) );
            }
        }
        else if( name == tokens::TimeFormat ) {
            auto value = common::iValueCast<common::StringValue>( command.getValue()->clone() );
            if( value && !value->get().empty() ) {
                setValueResult = core::HubSettings::get().setTimeFormat(
                    core::HubSettingsInfo::strToTimeFormat( value->get().c_str() ) );
            }
        }
        else if( name == tokens::DateFormat ) {
            auto value = common::iValueCast<common::StringValue>( command.getValue()->clone() );
            if( value && !value->get().empty() ) {
                setValueResult = core::HubSettings::get().setDateFormat(
                    core::HubSettingsInfo::strToDateFormat( value->get().c_str() ) );
            }
        }
        else if( name == tokens::LocationUUID ) {
            auto value = common::iValueCast<common::StringValue>( command.getValue()->clone() );
            if( value && !value->get().empty() ) {
                setValueResult = core::HubSettings::get().setLocationUuid( value->get().c_str() );
            }
        }
        else if( name == tokens::FirstStart ) {
            auto value = common::iValueCast<common::BoolValue>( command.getValue()->clone() );
            if( value ) {
                core::HubSettings::get().setFirstStart( value->get() );
            }
        }
    }
}

bool Core::hubSettingsApplyTemperatureScale( std::string dstScale ) {
    int counter = 0;
    gateways::enumerateItemsFromDb( [&counter]( const gateways::ItemInfo& itemInfo ) {
        counter++;
        return true;
    } );
    std::vector<std::string> done;
    done.reserve( counter );

    gateways::enumerateItemsFromDb( [&done, &dstScale]( const gateways::ItemInfo& itemInfoOld ) {
        gateways::ItemInfo itemInfo = itemInfoOld;

        auto item_id = itemInfo.getId();
        if( done.end() !=
            find_if( done.begin(), done.end(), [&item_id]( const std::string& id ) { return id == item_id; } ) ) {
            return true;
        }
        done.push_back( item_id );

        if( itemInfo.getValueType() != gateways::ItemValueType::Int &&
            itemInfo.getValueType() != gateways::ItemValueType::Float && !itemInfo.isScalable() ) {
            return true;
        }

        if( !itemInfoOld.isMinMax() ) {
            return true;
        }
        std::string srcScale =
            core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() );

        auto minmax = itemInfoOld.getMinMax();
        auto valueMin = itemInfoOld.getValue()->clone();
        float valMin = 0, valMax = 0;
        bool isNeedUpdateMinMax = false;
        if( valueMin ) {
            valMin = minmax.first;
            common::ScalesConverterFromFloatVisitor visitor(
                srcScale, dstScale, common::ScalesConverterVisitor::Precision, valMin );
            valueMin->accept( visitor );
            auto ivalMin = common::iValueCast<common::FloatValue>( std::move( valueMin ) );
            if( ivalMin && ivalMin->get() != valMin ) {
                valMin = ivalMin->get();
                isNeedUpdateMinMax = true;
            }
        }

        auto valueMax = itemInfoOld.getValue()->clone();
        if( valueMax ) {
            valMax = minmax.second;
            common::ScalesConverterFromFloatVisitor visitor(
                srcScale, dstScale, common::ScalesConverterVisitor::Precision, valMax );
            valueMax->accept( visitor );
            auto ivalMax = common::iValueCast<common::FloatValue>( std::move( valueMax ) );
            if( ivalMax && ivalMax->get() != valMax ) {
                valMax = ivalMax->get();
                isNeedUpdateMinMax = true;
            }
        }

        if( isNeedUpdateMinMax ) {
            itemInfo.setMinMax( { valMin, valMax } );

            auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
            if( !store ) {
                MODCOR_LOGE( "Can't save ItemInfo to storage for %s", itemInfo.getId().c_str() );
                return false;
            }
            store->remove( item_id );

            gateways::ObjectSerializer serializer;
            std::vector<char> vec = serializer.serialize( itemInfo );
            if( !( !vec.empty() && store->setBlob( itemInfo.getId(), std::move( vec ) ) ) ) {
                MODCOR_LOGE( "Can't save ItemInfo to storage for %s", itemInfo.getId().c_str() );
                return false;
            }
        }
        return true;
    } );
    return true;
}

bool Core::hubSettingsApplyTemperatureScaleSettings( std::string dstScale ) {
    int counter = 0;
    ::storage::DeviceSettings::get().enumerate( [&counter]( const gateways::DeviceSettingsInfo& settingInfo ) {
        counter++;
        return true;
    } );
    std::vector<std::string> done;
    done.reserve( counter );

    ::storage::DeviceSettings::get().enumerate( [&done,
                                                 &dstScale]( const gateways::DeviceSettingsInfo& settingInfoOrig ) {
        auto settingInfo = settingInfoOrig;
        auto settingId = settingInfo.getId();

        if( settingId.empty() ) {
            return true;
        }

        if( done.end() !=
            find_if( done.begin(), done.end(), [&settingId]( const std::string& id ) { return id == settingId; } ) ) {
            return true;
        }
        done.push_back( settingId );

        if( settingInfo.getValueType() != gateways::ItemValueType::Int &&
            settingInfo.getValueType() != gateways::ItemValueType::Float && !settingInfo.isScalable() ) {
            return true;
        }

        if( !settingInfoOrig.isMinMax() ) {
            return true;
        }
        std::string srcScale =
            core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() );

        auto minmax = settingInfoOrig.getMinMax();
        auto valueMin = settingInfoOrig.getValue()->clone();
        float valMin = 0, valMax = 0;
        bool isNeedUpdateMinMax = false;
        if( valueMin ) {
            valMin = minmax.first;
            common::ScalesConverterFromFloatVisitor visitor(
                srcScale, dstScale, common::ScalesConverterVisitor::Precision, valMin );
            valueMin->accept( visitor );
            auto ivalMin = common::iValueCast<common::FloatValue>( std::move( valueMin ) );
            if( ivalMin && ivalMin->get() != valMin ) {
                valMin = ivalMin->get();
                isNeedUpdateMinMax = true;
            }
        }

        auto valueMax = settingInfoOrig.getValue()->clone();
        if( valueMax ) {
            valMax = minmax.second;
            common::ScalesConverterFromFloatVisitor visitor(
                srcScale, dstScale, common::ScalesConverterVisitor::Precision, valMax );
            valueMax->accept( visitor );
            auto ivalMax = common::iValueCast<common::FloatValue>( std::move( valueMax ) );
            if( ivalMax && ivalMax->get() != valMax ) {
                valMax = ivalMax->get();
                isNeedUpdateMinMax = true;
            }
        }

        if( isNeedUpdateMinMax ) {
            settingInfo.setMinMax( { valMin, valMax } );

            ::storage::DeviceSettings::get().remove( settingId );
            if( !::storage::DeviceSettings::get().setInfo( settingInfo, lua_events::Synced, {} ) ) {
                MODCOR_LOGE( "Can't save ItemInfo to storage for %s", settingInfo.getId().c_str() );
                return false;
            }
        }
        return true;
    } );
    return true;
}

void Core::readTypeSpecificFieldsFromLua( lua_State* lua, ::gateways::ItemInfo& itemInfo ) {
    namespace lua_events = ::common::resources::lua_events;
    namespace tokens = ::common::resources::tokens;
    if( lua_events::WeeklyUserCodeIntervals == itemInfo.getName() ||
        lua_events::DailyUserCodeIntervals == itemInfo.getName() ) {
        if( auto elementsMaxNumberPerArray = createValueFromField( lua, lua_events::ElementsMaxNumber2 ) ) {
            MODCOR_LOGD( "%s filed is set to %s",
                         tokens::ElementsMaxNumberPerArray,
                         elementsMaxNumberPerArray->toJson( tokens::ElementsMaxNumberPerArray ).c_str() );
            itemInfo.setTypeSpecificField( tokens::ElementsMaxNumberPerArray, std::move( elementsMaxNumberPerArray ) );
        }
        else {
            MODCOR_LOGW( "%s filed is missing", lua_events::ElementsMaxNumber2 );
            return;
        }
        auto oneWeekDayCost = createValueFromField( lua, lua_events::OneWeekDayCost );
        if( !oneWeekDayCost ) {
            MODCOR_LOGD( "%s filed is missing", lua_events::OneWeekDayCost );
        }
        auto oneShiftedWeekDayCost = createValueFromField( lua, lua_events::OneShiftedWeekDayCost );
        if( !oneShiftedWeekDayCost ) {
            MODCOR_LOGD( "%s filed is missing", lua_events::OneShiftedWeekDayCost );
        }
        if( oneWeekDayCost && oneShiftedWeekDayCost ) {
            itemInfo.setTypeSpecificField( lua_events::OneWeekDayCost, std::move( oneWeekDayCost ) );
            itemInfo.setTypeSpecificField( lua_events::OneShiftedWeekDayCost, std::move( oneShiftedWeekDayCost ) );
        }
    }
    else if( lua_events::UserCodes == itemInfo.getName() ) {
        if( auto userCodeModes = createValueFromField( lua, lua_events::UserCodeModes ) ) {
            itemInfo.setTypeSpecificField( tokens::UserCodeModes, std::move( userCodeModes ) );
        }
    }
}

}  // namespace module

}  // namespace lua
