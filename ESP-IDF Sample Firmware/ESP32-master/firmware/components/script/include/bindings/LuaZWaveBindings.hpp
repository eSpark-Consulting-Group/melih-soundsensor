#pragma once

#include "LuaBinding.hpp"

#include <znet/znet_command_class_visitor.hpp>
#include <znet/converters/manufacturer_names_converter.hpp>
#include <znet/converters/user_code.hpp>

#include "converters/ZWaveInfoToValueConverter.h"
#include "converters/ZClassToValueConverter.h"
#include "converters/DeviceToValueConverter.h"

#include "IValue.h"
#include "IValueCast.h"
#include <IDInfo.h>

#include "BusCommandSetHandler.h"
#include "BusCommandReconstructNetwork.h"
#include "BusCommandHardReset.h"
#include "BusCommandStartInclude.h"
#include "BusCommandStartLearn.h"
#include "BusCommandStartExclude.h"
#include "BusCommandStartRediscover.h"
#include "BusCommandStopInclude.h"
#include "BusCommandStopSecurityInclude.h"
#include "BusCommandStopExclude.h"
#include "BusCommandStopLearn.h"
#include "BusCommandS2SelectKey.h"
#include "BusCommandS2SetPublicKey.h"
#include "BusCommandSetBarrierSignal.h"
#include "BusCommandSetDevicePollIntervals.h"
#include "BusCommandSetValue.h"
#include "BusCommandSetValueMulticast.h"
#include "BusCommandSetThermostatMode.hpp"
#include "BusCommandSetUserCode.h"
#include "BusCommandScheduleEntryLock.hpp"
#include "BusCommandRequestBarrierSignal.h"
#include "BusCommandRequestValue.h"
#include "BusCommandRequestUserCode.h"
#include "BusCommandRequestDoorLockState.hpp"
#include "BusCommandRequestValueTermostatSetpoint.h"
#include "BusCommandRequestAlarmV1Value.hpp"
#include "BusCommandSoundSwitch.hpp"
#include "BusCommandGetBackgroundRSSI.h"
#include "BusCommandSwitchLevelChange.hpp"
#include "BusCommandUserCodeV2.hpp"
#include "BusCommandLockAntiTheft.h"
#include "BusCommandLockAntiTheftDefault.h"
#include "BusCommandUnlockAntiTheft.h"
#include "BusCommandUnlockAntiTheftDefault.h"
#include "gateways/commands/BusCommandSendIdentify.hpp"
#include <gateways/commands/BusCommandZwaveProvisioningUpdate.h>
#include <gateways/commands/BusCommandZwaveProvisioningGet.h>
#include <gateways/commands/BusCommandUpdatesNodesFirmware.hpp>
#include <gateways/commands/BusCommandCheckNodeValues.hpp>
#include <gateways/commands/BusCommandCheckReachableStatus.hpp>
#include <configuration/commands/BusCommandRequestConfiguration.h>
#include <configuration/commands/BusCommandRemoveConfiguration.h>
#include <configuration/commands/BusCommandSetConfiguration.h>
#include <configuration/commands/BusCommandAddConfiguration.h>
#include <configuration/commands/BusCommandRequestDoorLockConfiguration.hpp>
#include <configuration/commands/BusCommandSetDoorLockConfiguration.hpp>
#include <configuration/commands/BusCommandResetAllConfigurationValues.hpp>

#include "Lua.h"
#include "ZWaveGateway.h"
#include <CommonResources.h>

static constexpr auto ZNET_NAME = "znet";

using namespace std::chrono_literals;

//#define SIMPLE_ZWBND_DEBUG
#ifdef SIMPLE_ZWBND_DEBUG

#include <esp_log.h>
#include "EspLogTags.h"

#define ZWBND_LOGI( ... ) ESP_LOGI(::esp_log_tags::LUA, __VA_ARGS__ )
#define ZWBND_LOGE( ... ) ESP_LOGE(::esp_log_tags::LUA, __VA_ARGS__ )
#define ZWBND_LOGW( ... ) ESP_LOGW(::esp_log_tags::LUA, __VA_ARGS__ )
#define ZWBND_LOGD( ... ) ESP_LOGD(::esp_log_tags::LUA, __VA_ARGS__ )

#else

#define ZWBND_LOGD( ... ) \
    do {                  \
    } while( false )
#define ZWBND_LOGI( ... ) \
    do {                  \
    } while( false )
#define ZWBND_LOGE( ... ) \
    do {                  \
    } while( false )
#define ZWBND_LOGW( ... ) \
    do {                  \
    } while( false )

#endif

namespace lua {
namespace module {
namespace bindings {
namespace zwave {

using namespace common::resources;

// Todo: Check if this binding is used
class Join : public LuaBinding<bool( std::string )> {
public:
    bool operator()( const std::string& filename ) const {
        ZWBND_LOGD( "zwave::join( filename=%s )", filename.c_str() );
        return m_ctx->getExecutor().busJoin( m_ctx->getModule(), "extensions.zwave", filename.c_str() );
    }
};

class Subscribe : public LuaBinding<void( std::string )> {
public:
    void operator()( std::string scriptName ) const {
        ZWBND_LOGD( "zwave::subscribe( scriptName=%s )", scriptName.c_str() );
        m_ctx->getExecutor().send<bus::command::SetHandler>( ZNET_NAME, bus::BusAddrInvalid, scriptName );
    }
};

class Unsubscribe : public LuaBinding<void( std::string )> {
public:
    void operator()( std::string scriptName ) const {
        ZWBND_LOGD( "zwave::unsubscribe( scriptName=%s )", scriptName.c_str() );
        m_ctx->getExecutor().send<bus::command::SetHandler>( ZNET_NAME, bus::BusAddrInvalid, scriptName );
    }
};

class GetInfo : public LuaBinding<std::shared_ptr<::common::TableValue>()> {
public:
    std::shared_ptr<::common::TableValue> operator()() const {
        ZWBND_LOGD( "zwave::get_info()" );

        auto deviceTable = std::make_shared<::common::TableValue>();
        ::lua::module::converters::zwave::ZWaveInfoToValueConverter converter{ *deviceTable };
        converter();
        return deviceTable;
    }
};

class GetManufacturerName : public LuaBinding<std::string( int )> {
public:
    std::string operator()( int manufacturerId ) const {
        ZWBND_LOGD( "zwave::get_manufacturer_name( manufacturer) = %d", manufacturerId );
        return znet::manufacturer_names_converter::manufacturerIdToName( manufacturerId );
    }
};

class GetAllNodesIds : public LuaBinding<std::vector<uint8_t>()> {
public:
    std::vector<uint8_t> operator()() const {
        ZWBND_LOGD( "zwave::get_all_nodes_ids()" );
        std::vector<uint8_t> nodeIds( gateways::zwave::Gateway::get().getDevicesCount() );
        for( size_t i = 0; i < nodeIds.size(); ++i ) {
            auto device = gateways::zwave::Gateway::get().getDeviceByIndex( i );
            nodeIds[i] = device->getId();
        }
        return nodeIds;
    }
};

class GetBackgroundRSSI : public LuaBinding<void()> {
public:
    void operator()() const {
        ZWBND_LOGD( "zwave::get_background_rssi()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::GetBackgrounRSSI>() );
    }
};

class GetCommandClass : public LuaBinding<std::shared_ptr<::common::TableValue>( int, int, int )> {
public:
    std::shared_ptr<::common::TableValue> operator()( int nodeId, int channelId, int classId ) const {
        ZWBND_LOGD( "zwave::get_command_class( nodeId=%d, channelId=%d, "
                    "classId=%d )",
                    nodeId,
                    channelId,
                    classId );
        auto device = gateways::zwave::Gateway::get().getDevice( nodeId );
        if( !device ) {
            ZWBND_LOGE( "Cannot find a device with nodeId=%d", nodeId );
            return {};
        }

        auto commandClass = device->getCommandClass( channelId, classId );
        if( !commandClass ) {
            ZWBND_LOGE( "Cannot find command class classId=0x%X, channelId=%d, nodeId=%d", classId, channelId, nodeId );
            return {};
        }

        auto ccTable = std::make_shared<::common::TableValue>();
        ::lua::module::converters::zwave::ZClassToValueConverter converter{ *ccTable };
        ::znet::visitors::visit( converter, *commandClass );
        return ccTable;
    }
};

class GetNode : public LuaBinding<std::shared_ptr<::common::TableValue>( int nodeId )> {
public:
    std::shared_ptr<::common::TableValue> operator()( int nodeId ) const {
        ZWBND_LOGD( "zwave::get_node( nodeId=%d )", nodeId );
        auto device = ::gateways::zwave::Gateway::get().getDevice( nodeId );
        if( !device ) {
            REMOTE_LOGE( __FUNCTION__, "Cannot find device with nodeId=%d", nodeId );
            return {};
        }

        auto channelList = std::make_unique<::common::ArrayValue>();
        for( uint8_t channelId = 0; channelId < device->getChannelsCount(); ++channelId ) {
            auto classList = std::make_unique<::common::ArrayValue>();
            for( uint8_t classId = 0; classId < device->getCommandClassCount( channelId ); ++classId ) {
                znet::ZNode::ClassPtr cc = device->getCommandClassByIndex( channelId, classId );
                if( !cc ) {
                    continue;
                }
                // TODO: Temporary solution. Do not create items related to CentralScene for SWF MCZ1
                if( device->getManufacturerId() == 622 && device->getProductType() == 19779 &&
                    device->getProductId() == 23089 && cc->getId() == ZNET_COMMAND_CLASS_CENTRAL_SCENE )
                    continue;

                auto ccTable = std::make_unique<::common::TableValue>();
                ::lua::module::converters::zwave::ZClassToValueConverter converter{ *ccTable };
                ::znet::visitors::visit( converter, *cc );
                classList->push_back( std::move( ccTable ) );
            }

            auto channel = std::make_unique<::common::TableValue>();
            channel->insert( common::resources::lua_events::Id, std::make_unique<::common::IntValue>( channelId ) );
            channel->insert( common::resources::lua_events::Classes, std::move( classList ) );
            channelList->push_back( std::move( channel ) );
        }

        auto deviceTable = std::make_shared<::common::TableValue>();
        ::lua::module::converters::zwave::DeviceToValueConverter converter{ *deviceTable };
        converter( *device );

        deviceTable->insert( common::resources::lua_events::Channels, std::move( channelList ) );
        return deviceTable;
    }
};

class GetNodeInfo : public LuaBinding<std::shared_ptr<::common::TableValue>( int nodeId )> {
public:
    std::shared_ptr<::common::TableValue> operator()( int nodeId ) const {
        ZWBND_LOGD( "zwave::get_node_info( nodeId=%d )", nodeId );
        auto device = ::gateways::zwave::Gateway::get().getDevice( nodeId );
        if( !device ) {
            REMOTE_LOGE( __FUNCTION__, "Cannot find device with nodeId=%d", nodeId );
            return {};
        }

        auto deviceTable = std::make_shared<::common::TableValue>();
        ::lua::module::converters::zwave::DeviceToValueConverter converter{ *deviceTable };
        converter( *device );
        return deviceTable;
    }
};

class ReconstructNetwork : public LuaBinding<void()> {
public:
    void operator()() const {
        ZWBND_LOGD( "zwave::reconstruct_network()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::ReconstructNetwork>() );
    }
};

class RequestConfigurationValue : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int paramNumber ) const {
        ZWBND_LOGD( "zwave::request_configuration_value( nodeId=%d, arg=%d )", nodeId, paramNumber );
        auto bus_command = std::make_unique<::configuration::commands::RequestConfiguration>( nodeId, paramNumber );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class RequestDoorLockConfiguration : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int channelId ) const {
        ZWBND_LOGD( "zwave::request_door_lock_configuration( nodeId=%d, channelId=%d )", nodeId, channelId );
        m_ctx->getExecutor().send(
            ZNET_NAME, std::make_unique<::configuration::commands::RequestDoorLockConfiguration>( nodeId, channelId ) );
    }
};

class RequestDoorLockState : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int channelId ) const {
        ZWBND_LOGD( "zwave::request_door_lock_state( nodeId=%d, channelId=%d )", nodeId, channelId );
        m_ctx->getExecutor().send( ZNET_NAME,
                                   std::make_unique<::bus::command::RequestDoorLockState>( nodeId, channelId ) );
    }
};

class RequestNodeValues : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int requestDelay ) const {
        ZWBND_LOGD( "zwave::request_node_values( nodeId=%d, requestDelay=%d )", nodeId, requestDelay );
        auto bus_command = std::make_unique<::bus::command::RequestValue>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_commandClassId = 0;
        bus_command->m_channelId = 0;
        bus_command->m_requestDelay = std::chrono::milliseconds( requestDelay );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class RequestValue : public LuaBinding<void( int, int, int )> {
public:
    void operator()( int nodeId, int classId, int channelId ) const {
        ZWBND_LOGD( "zwave::request_value( nodeId=%d, classId=%d, channel=%d )", nodeId, classId, channelId );
        auto bus_command = std::make_unique<::bus::command::RequestValue>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_commandClassId = classId;
        bus_command->m_channelId = channelId;
        bus_command->m_requestDelay = std::chrono::milliseconds( 0 );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class RequestUserCode : public LuaBinding<void( int, int, int )> {
public:
    void operator()( int nodeId, int channelId, int userId ) const {
        ZWBND_LOGD( "zwave::request_user_code( nodeId=%d, channelId=%d, userId=%d )", nodeId, channelId, userId );
        auto bus_command = std::make_unique<::bus::command::RequestUserCode>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_channelId = channelId;
        bus_command->m_user_id = userId;
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class RequestMasterCode : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int channelId ) const {
        ZWBND_LOGD( "zwave::request_master_code( nodeId=%d, channelId=%d )", nodeId, channelId );
        m_ctx->getExecutor().send<::bus::command::UserCodeV2Operation>(
            ZNET_NAME, ::bus::command::UserCodeV2Operation::REQUEST_MASTER_CODE, nodeId, channelId );
    }
};

class RequestUsesCodesKeypadMode : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int channelId ) const {
        ZWBND_LOGD( "zwave::request_user_codes_keypad_mode( nodeId=%d, channelId=%d )", nodeId, channelId );
        m_ctx->getExecutor().send<::bus::command::UserCodeV2Operation>(
            ZNET_NAME, ::bus::command::UserCodeV2Operation::REQUEST_KEYPAD_MODE, nodeId, channelId );
    }
};

class RequestUserCodesChecksum : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int channelId ) const {
        ZWBND_LOGD( "zwave::request_user_codes_checksum( nodeId=%d, channelId=%d )", nodeId, channelId );
        m_ctx->getExecutor().send<::bus::command::UserCodeV2Operation>(
            ZNET_NAME, ::bus::command::UserCodeV2Operation::REQUEST_CHECKSUM, nodeId, channelId );
    }
};

class RequestWeekDayEntryLockSchedule : public LuaBinding<void( int, int, int, int )> {
public:
    void operator()( int nodeId, int channelId, int userId, int slotId ) const {
        ZWBND_LOGD( "zwave::request_week_day_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d )",
                    nodeId,
                    channelId,
                    userId,
                    slotId );
        m_ctx->getExecutor().send(
            ZNET_NAME,
            std::make_unique<::bus::command::RequestWeekDayEntryLockSchedule>( nodeId, channelId, userId, slotId ) );
    }
};

class RequestDailyRepeatingEntryLockSchedule : public LuaBinding<void( int, int, int, int )> {
public:
    void operator()( int nodeId, int channelId, int userId, int slotId ) const {
        ZWBND_LOGD(
            "zwave::request_daily_repeating_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d )",
            nodeId,
            channelId,
            userId,
            slotId );
        m_ctx->getExecutor().send( ZNET_NAME,
                                   std::make_unique<::bus::command::RequestDailyRepeatingEntryLockSchedule>(
                                       nodeId, channelId, userId, slotId ) );
    }
};

class RequestYearDayEntryLockSchedule : public LuaBinding<void( int, int, int, int )> {
public:
    void operator()( int nodeId, int channelId, int userId, int slotId ) const {
        ZWBND_LOGD( "zwave::request_year_day_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d )",
                    nodeId,
                    channelId,
                    userId,
                    slotId );
        m_ctx->getExecutor().send(
            ZNET_NAME,
            std::make_unique<::bus::command::RequestYearDayEntryLockSchedule>( nodeId, channelId, userId, slotId ) );
    }
};

class RequestSetpointValue : public LuaBinding<void( int, int, int )> {
public:
    void operator()( int nodeId, int channelId, int setpointType ) const {
        ZWBND_LOGD( "zwave::request_setpoint_value( nodeId=%d, channelId=%d, setpointType=%d )",
                    nodeId,
                    channelId,
                    setpointType );
        auto bus_command = std::make_unique<::bus::command::RequestValueTermostatSetpoint>();
        bus_command->m_commandClassId = ZNET_COMMAND_CLASS_THERMOSTAT_SETPOINT;
        bus_command->m_nodeId = nodeId;
        bus_command->m_channelId = channelId;
        bus_command->m_setpointType = setpointType;
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class RequestSoundSwitchTonePlay : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int channelId ) const {
        ZWBND_LOGD( "zwave::request_sound_switch_tone_play( nodeId=%d, channelId=%d )", nodeId, channelId );
        m_ctx->getExecutor().send( ZNET_NAME,
                                   std::make_unique<::bus::command::SoundSwitchTonePlayGet>( nodeId, channelId ) );
    }
};

class RequestSoundSwitchConfiguration : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int channelId ) const {
        ZWBND_LOGD( "zwave::request_sound_switch_configuration( nodeId=%d, channelId=%d )", nodeId, channelId );
        m_ctx->getExecutor().send( ZNET_NAME,
                                   std::make_unique<::bus::command::SoundSwitchConfigrationGet>( nodeId, channelId ) );
    }
};

class RequestAlarmV1Value : public LuaBinding<void( int, int, int )> {
public:
    void operator()( int nodeId, int channelId, int type ) const {
        ZWBND_LOGD( "zwave::request_alarm_v1_value( nodeId=%d, channelId=%d, type=%d )", nodeId, channelId, type );
        m_ctx->getExecutor().send( ZNET_NAME,
                                   std::make_unique<::bus::command::RequestAlarmV1Value>( nodeId, channelId, type ) );
    }
};

class Reset : public LuaBinding<void()> {
public:
    void operator()() const {
        ZWBND_LOGD( "zwave::reset()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::HardReset>() );
    }
};

class ResetConfigurationValue : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int paramNum ) const {
        ZWBND_LOGD( "zwave::reset_configuration_value( nodeId=%d, paramNum=%d )", nodeId, paramNum );
        auto bus_command =
            std::make_unique<::configuration::commands::SetConfiguration>( nodeId, paramNum, 0, 0, true );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class ResetAllConfigurationValues : public LuaBinding<void( int )> {
public:
    void operator()( int nodeId ) const {
        ZWBND_LOGD( "zwave::reset_all_configuration_values( nodeId=%d )", nodeId );
        auto bus_command = std::make_unique<::configuration::commands::ResetAllConfigurationValues>( nodeId );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetAuthMode : public LuaBinding<void( std::vector<std::string> )> {
public:
    void operator()( std::vector<std::string> authModes ) const {
        ZWBND_LOGD( "zwave::set_auth_mode( authModes=[...] )" );
        auto bus_command = std::make_unique<::bus::command::S2SelectKey>();
        bus_command->m_authModes = std::move( authModes );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetConfigurationValue : public LuaBinding<void( int, int, int, int64_t )> {
public:
    void operator()( int nodeId, int argNumber, int argSize, int64_t argValue ) const {
        ZWBND_LOGD( "zwave::set_configuration_value( nodeId=%d, argNumber=%d, argSize=%d, "
                    "argValue=%" PRId64 " )",
                    nodeId,
                    argNumber,
                    argSize,
                    argValue );
        auto bus_command =
            std::make_unique<::configuration::commands::SetConfiguration>( nodeId, argNumber, argSize, argValue, true );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetDeviceSpecificKey : public LuaBinding<void( int )> {
public:
    void operator()( int key ) const {
        ZWBND_LOGD( "zwave::set_device_specific_key( key=%d )", key );
        auto bus_command = std::make_unique<::bus::command::S2SetPublicKey>();
        bus_command->m_key = key;
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetDoorLockMode : public LuaBinding<void( int, int, std::string )> {
public:
    void operator()( int nodeId, int channelId, std::string value ) const {
        ZWBND_LOGD(
            "zwave::set_door_lock_mode( nodeId=%d, channelId=%d, value=%s )", nodeId, channelId, value.c_str() );
        auto bus_command = std::make_unique<::bus::command::SetValue>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_channelId = channelId;
        bus_command->m_commandClassId = ZNET_COMMAND_CLASS_DOOR_LOCK;
        bus_command->m_value = std::make_unique<::common::StringValue>( std::move( value ) );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetDoorLockConfiguration : public LuaBinding<void( int, int, std::unique_ptr<::common::TableValue> )> {
public:
    void operator()( int nodeId, int channelId, std::unique_ptr<::common::TableValue> configurationPtr ) const {
        ZWBND_LOGD( "zwave::set_door_lock_configuration( nodeId=%d, channelId=%d, configuration=%s )",
                    nodeId,
                    channelId,
                    configurationPtr->toJson().c_str() );
        namespace lua_res = ::common::resources::lua_events;
        auto& configuration = *configurationPtr;
        m_ctx->getExecutor().send( ZNET_NAME,
                                   std::make_unique<::configuration::commands::SetDoorLockConfiguration>(
                                       nodeId,
                                       channelId,
                                       parseHandleIds( configuration[lua_res::EnabledInsideHandleIds] ),
                                       parseHandleIds( configuration[lua_res::EnabledOutsideHandleIds] ),
                                       parseLockTimeout( configuration[lua_res::LockTimeout] ),
                                       parseExtendedConfiguration( configuration ) ) );
    }

    static std::vector<uint8_t> parseHandleIds(::common::IValue* value ) {
        std::vector<uint8_t> result;
        if( auto handleIdArray = ::common::iValueCast<::common::ArrayValue>( value ) ) {
            for( const auto& handleIdValue : *handleIdArray ) {
                if( auto handleId = ::common::iValueCast<::common::IntValue>( handleIdValue.get() ) ) {
                    result.push_back( handleId->get() );
                }
            }
        }
        return result;
    }

    static std::optional<uint16_t> parseLockTimeout(::common::IValue* value ) {
        auto lockTimeoutValue = ::common::iValueCast<::common::IntValue>( value );
        return lockTimeoutValue ? std::optional<uint16_t>{ lockTimeoutValue->get() } : std::nullopt;
    }

    static std::optional<::configuration::ExtDoorLockConfiguration> parseExtendedConfiguration(
        ::common::TableValue& conf ) {
        namespace lua_res = ::common::resources::lua_events;
        auto autoRelockTimeout = ::common::iValueCast<::common::IntValue>( conf[lua_res::AutoRelockTimeout] );
        auto holdAndReleaseTimeout = ::common::iValueCast<::common::IntValue>( conf[lua_res::HoldAndReleaseTimeout] );
        auto twistAssistEnabled = ::common::iValueCast<::common::BoolValue>( conf[lua_res::TwistAssistEnabled] );
        auto blockToBlockEnabled = ::common::iValueCast<::common::BoolValue>( conf[lua_res::BlockToBlockEnabled] );
        if( autoRelockTimeout && holdAndReleaseTimeout && twistAssistEnabled && blockToBlockEnabled ) {
            return ::configuration::ExtDoorLockConfiguration{
                .m_autoRelockTimeout = static_cast<uint16_t>( autoRelockTimeout->get() ),
                .m_holdAndReleaseTimeout = static_cast<uint16_t>( holdAndReleaseTimeout->get() ),
                .m_twistAssistEnabled = twistAssistEnabled->get(),
                .m_blockToBlockEnabled = blockToBlockEnabled->get()
            };
        }
        else {
            return std::nullopt;
        }
    }
};

class SetRgbValue : public LuaBinding<void( int, int, int, std::unique_ptr<::common::RgbValue> )> {
public:
    void operator()( int nodeId, int classId, int channelId, std::unique_ptr<::common::RgbValue> rgbValue ) const {
        ZWBND_LOGD( "zwave::set_rgb_value( nodeId=%d, classId=%d, channelId=%d, rgbValue=%s )",
                    nodeId,
                    classId,
                    channelId,
                    rgbValue->toJson().c_str() );
        auto bus_command = std::make_unique<::bus::command::SetValue>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_channelId = channelId;
        bus_command->m_commandClassId = classId;
        bus_command->m_value = std::move( rgbValue );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetRgbValueMulticast
: public LuaBinding<void( std::vector<uint8_t>, int, int, std::unique_ptr<::common::RgbValue>, std::string )> {
public:
    void operator()( std::vector<uint8_t> nodeIdList,
                     int classId,
                     int channelId,
                     std::unique_ptr<::common::RgbValue> rgbValue,
                     std::string mode ) const {
        ZWBND_LOGD( "zwave::SetValueMulticast( nodeIdList=[...], classId=%d, channelId=%d, value={...}, mode=%s )",
                    classId,
                    channelId,
                    mode.c_str() );
        auto bus_command = std::make_unique<::bus::command::SetValueMulticast>();
        bus_command->m_channelId = channelId;
        bus_command->m_commandClassId = classId;
        bus_command->m_value = std::move( rgbValue );
        bus_command->m_nodeId = std::move( nodeIdList );
        bus_command->m_mode = mode;
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }

    void operator()( std::vector<uint8_t> nodeIdList,
                     int classId,
                     int channelId,
                     std::unique_ptr<::common::RgbValue> rgbValue ) const {
        operator()( nodeIdList, classId, channelId, std::move( rgbValue ), "chip_singlecasts" );
    }
};

class SetThermostatSetbackValue
: public LuaBinding<void( int, int, std::string, std::string, std::unique_ptr<::common::FloatValue> )> {
public:
    void operator()( int nodeId,
                     int channelId,
                     std::string overrideType,
                     std::string mode,
                     std::unique_ptr<::common::FloatValue> value ) const {
        ZWBND_LOGD( "zwave::set_thermostat_setback_value( nodeId=%d, channelId=%d, overrideType=%s, mode=%s, "
                    "floatValue={...} )",
                    nodeId,
                    channelId,
                    overrideType.c_str() ? overrideType.c_str() : "",
                    mode.c_str() ? mode.c_str() : "" );

        auto tblVal = std::make_unique<::common::TableValue>();
        if( tblVal ) {
            tblVal->insert( "overrideType", std::make_unique<::common::StringValue>( overrideType ) );
            tblVal->insert( "mode", std::make_unique<::common::StringValue>( mode ) );
            tblVal->insert( "value", std::move( value ) );
        }

        auto bus_command = std::make_unique<::bus::command::SetValue>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_channelId = channelId;
        bus_command->m_commandClassId = ZNET_COMMAND_CLASS_THERMOSTAT_SETBACK;
        bus_command->m_value = std::move( tblVal );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetThermostatMode : public LuaBinding<void( int, int, int, std::optional<std::string> )> {
public:
    void operator()( int nodeId, int channelId, int mode, std::optional<std::string> manufacturerSpecificData ) {
        ZWBND_LOGD( "zwave::set_thermostat_mode( nodeId=%d, channelId=%d, mode=%d, manufacturerSpecificData=%s )",
                    nodeId,
                    channelId,
                    mode,
                    manufacturerSpecificData ? manufacturerSpecificData->c_str() : "null" );
        m_ctx->getExecutor().send( ZNET_NAME,
                                   std::make_unique<::bus::command::SetThermostatMode>(
                                       nodeId, channelId, mode, std::move( manufacturerSpecificData ) ) );
    }
};

class SetSetpointValue : public LuaBinding<void( int, int, int, int, std::unique_ptr<::common::TableValue> )> {
public:
    void operator()( int nodeId,
                     int classId,
                     int channelId,
                     int setpointType,
                     std::unique_ptr<::common::TableValue> value ) const {
        ZWBND_LOGD( "zwave::set_setpoint_value( nodeId=%d, classId=%d, channelId=%d, setpointType=%d, "
                    "scalableValue={...} )",
                    nodeId,
                    classId,
                    channelId,
                    setpointType );
        if( !value ) {
            ZWBND_LOGE( "zwave:set_setpoint_value value is empty" );
            return;
        }

        if( !value->isKeyExist( tokens::Type ) ) {
            value->insert( tokens::Type,  // setpoint_type
                           std::make_unique<common::IntValue>( setpointType ) );
        }

        auto bus_command = std::make_unique<::bus::command::SetValue>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_channelId = channelId;
        bus_command->m_commandClassId = classId;
        bus_command->m_value = std::move( value );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetValue : public LuaBinding<void( int, int, int, std::unique_ptr<::common::IValue> )> {
public:
    void operator()( int nodeId, int classId, int channelId, std::unique_ptr<::common::IValue> value ) const {
        ZWBND_LOGD( "zwave::SetValue( nodeId=%d, classId=%d, channelId=%d, value={...} )", nodeId, classId, channelId );
        auto bus_command = std::make_unique<::bus::command::SetValue>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_channelId = channelId;
        bus_command->m_commandClassId = classId;
        bus_command->m_value = std::move( value );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetValueMulticast
: public LuaBinding<void( std::vector<uint8_t>, int, int, std::unique_ptr<::common::IValue>, std::string )> {
public:
    void operator()( std::vector<uint8_t> nodeIdList,
                     int classId,
                     int channelId,
                     std::unique_ptr<::common::IValue> value,
                     std::string mode ) const {
        ZWBND_LOGD( "zwave::SetValueMulticast( nodeIdList=[...], classId=%d, channelId=%d, value={...}, mode=%s )",
                    classId,
                    channelId,
                    mode.c_str() );
        auto bus_command = std::make_unique<::bus::command::SetValueMulticast>();
        bus_command->m_channelId = channelId;
        bus_command->m_commandClassId = classId;
        bus_command->m_value = std::move( value );
        bus_command->m_nodeId = std::move( nodeIdList );
        bus_command->m_mode = mode;
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }

    void operator()( std::vector<uint8_t> nodeIdList,
                     int classId,
                     int channelId,
                     std::unique_ptr<::common::IValue> value ) const {
        operator()( nodeIdList, classId, channelId, std::move( value ), "chip_singlecasts" );
    }
};

class SetUserCode : public LuaBinding<void( int, int, int, std::string, std::string )> {
public:
    void operator()( int nodeId, int channelId, int userId, std::string userIdStatus, std::string userCode ) const {
        ZWBND_LOGD( "zwave::set_user_code( nodeId=%d, channelId=%d, userId=%d, userIdStatus=%s, userCode=%s )",
                    nodeId,
                    channelId,
                    userId,
                    userIdStatus.c_str(),
                    userCode.c_str() );
        auto bus_command = std::make_unique<::bus::command::SetUserCode>();
        bus_command->m_nodeId = nodeId;
        bus_command->m_channelId = channelId;
        bus_command->m_user_id = userId;
        bus_command->m_user_id_status = std::move( userIdStatus );
        bus_command->m_user_code = std::move( userCode );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetMasterCode : public LuaBinding<void( int, int, std::string )> {
public:
    void operator()( int nodeId, int channelId, std::string code ) const {
        ZWBND_LOGD( "zwave::set_master_code( nodeId=%d, channelId=%d, code=%s )", nodeId, channelId, code.c_str() );
        m_ctx->getExecutor().send<::bus::command::UserCodeV2Operation>(
            ZNET_NAME, ::bus::command::UserCodeV2Operation::SET_MASTER_CODE, nodeId, channelId, std::move( code ) );
    }
};

class SetUserCodesKeypadMode : public LuaBinding<void( int, int, std::string )> {
public:
    void operator()( int nodeId, int channelId, std::string mode ) const {
        ZWBND_LOGD(
            "zwave::set_user_codes_keypad_mode( nodeId=%d, channelId=%d, mode=%s )", nodeId, channelId, mode.c_str() );
        m_ctx->getExecutor().send<::bus::command::UserCodeV2Operation>(
            ZNET_NAME, ::bus::command::UserCodeV2Operation::SET_KEYPAD_MODE, nodeId, channelId, std::move( mode ) );
    }
};

class SetDailyRepeatingEntryLockSchedule
: public LuaBinding<void( int, int, int, int, std::unique_ptr<::common::TableValue> )> {
public:
    void operator()( int nodeId,
                     int channelId,
                     int userId,
                     int slotId,
                     std::unique_ptr<::common::TableValue> schedule ) const {
        if( schedule && !schedule->empty() ) {
            auto intervalJson = schedule->toJson( "schedule" );
            ZWBND_LOGD(
                "zwave::set_daily_repeating_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d, "
                "schedule=%s )",
                nodeId,
                channelId,
                userId,
                slotId,
                intervalJson.c_str() );
            m_ctx->getExecutor().send( ZNET_NAME,
                                       std::make_unique<::bus::command::SetDailyRepeatingEntryLockSchedule>(
                                           nodeId, channelId, userId, slotId, std::move( schedule ) ) );
        }
        else {
            ZWBND_LOGD(
                "zwave::set_daily_repeating_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d, "
                "schedule=nil )",
                nodeId,
                channelId,
                userId,
                slotId );
            m_ctx->getExecutor().send( ZNET_NAME,
                                       std::make_unique<::bus::command::RemoveDailyRepeatingEntryLockSchedule>(
                                           nodeId, channelId, userId, slotId ) );
        }
    }
};

class SetWeekDayEntryLockSchedule
: public LuaBinding<void( int, int, int, int, std::unique_ptr<::common::TableValue> )> {
public:
    void operator()( int nodeId,
                     int channelId,
                     int userId,
                     int slotId,
                     std::unique_ptr<::common::TableValue> schedule ) const {
        if( schedule && !schedule->empty() ) {
            auto intervalJson = schedule->toJson( "schedule" );
            ZWBND_LOGD(
                "zwave::set_week_day_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d, schedule=%s )",
                nodeId,
                channelId,
                userId,
                slotId,
                intervalJson.c_str() );
            m_ctx->getExecutor().send( ZNET_NAME,
                                       std::make_unique<::bus::command::SetWeekDayEntryLockSchedule>(
                                           nodeId, channelId, userId, slotId, std::move( schedule ) ) );
        }
        else {
            ZWBND_LOGD( "zwave::set_week_day_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d, "
                        "schedule=nil )",
                        nodeId,
                        channelId,
                        userId,
                        slotId );
            m_ctx->getExecutor().send(
                ZNET_NAME,
                std::make_unique<::bus::command::RemoveWeekDayEntryLockSchedule>( nodeId, channelId, userId, slotId ) );
        }
    }
};

class SetYearDayEntryLockSchedule
: public LuaBinding<void( int, int, int, int, std::unique_ptr<::common::TableValue> )> {
public:
    void operator()( int nodeId,
                     int channelId,
                     int userId,
                     int slotId,
                     std::unique_ptr<::common::TableValue> schedule ) const {
        if( schedule && !schedule->empty() ) {
            auto intervalJson = schedule->toJson( "schedule" );
            ZWBND_LOGD(
                "zwave::set_year_day_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d, schedule=%s )",
                nodeId,
                channelId,
                userId,
                slotId,
                intervalJson.c_str() );
            m_ctx->getExecutor().send( ZNET_NAME,
                                       std::make_unique<::bus::command::SetYearDayEntryLockSchedule>(
                                           nodeId, channelId, userId, slotId, std::move( schedule ) ) );
        }
        else {
            ZWBND_LOGD( "zwave::set_year_day_entry_lock_schedule( nodeId=%d, channelId=%d, usedId=%d, slotId=%d, "
                        "schedule=nil )",
                        nodeId,
                        channelId,
                        userId,
                        slotId );
            m_ctx->getExecutor().send(
                ZNET_NAME,
                std::make_unique<::bus::command::RemoveYearDayEntryLockSchedule>( nodeId, channelId, userId, slotId ) );
        }
    }
};

class SetSoundSwitchTonePlay : public LuaBinding<void( int, int, int, int )> {
public:
    void operator()( int nodeId, int channelId, int toneId, int volume ) const {
        ZWBND_LOGD( "zwave::set_sound_switch_tone_play( nodeId=%d, channelId=%d, toneId=%d, volume=%d ) const",
                    nodeId,
                    channelId,
                    toneId,
                    volume );
        m_ctx->getExecutor().send(
            ZNET_NAME, std::make_unique<::bus::command::SoundSwitchTonePlaySet>( nodeId, channelId, toneId, volume ) );
    }
};

class SetSoundSwitchConfiguration : public LuaBinding<void( int, int, int, int )> {
public:
    void operator()( int nodeId, int channelId, int volume, int defaultToneId ) const {
        ZWBND_LOGD( "zwave::set_sound_switch_configuration( nodeId=%d, channelId=%d, volume=%d, defaultToneId=%d )",
                    nodeId,
                    channelId,
                    volume,
                    defaultToneId );
        m_ctx->getExecutor().send(
            ZNET_NAME,
            std::make_unique<::bus::command::SoundSwitchConfigrationSet>( nodeId, channelId, volume, defaultToneId ) );
    }
};

class SetSwitchMultilevelLevel
: public Overloaded<LuaBinding<void( int, int, std::string, int )>, LuaBinding<void( int, int, std::string )>> {
public:
    void operator()( int nodeId, int channelId, std::string command, int duration ) const {
        ZWBND_LOGD( "zwave::switch_multilevel_level_change( nodeId=%d, channelId=%d, command=%s, duration=%d )",
                    nodeId,
                    channelId,
                    command.c_str(),
                    duration );
        return m_ctx->getExecutor().send<::bus::command::SwitchLevelChange>(
            ZNET_NAME, nodeId, channelId, std::move( command ), std::chrono::seconds{ duration } );
    }

    void operator()( int nodeId, int channelId, std::string command ) const {
        ZWBND_LOGD( "zwave::switch_multilevel_level_change( nodeId=%d, channelId=%d, command=%s )",
                    nodeId,
                    channelId,
                    command.c_str() );
        return m_ctx->getExecutor().send<::bus::command::SwitchLevelChange>(
            ZNET_NAME, nodeId, channelId, std::move( command ), std::nullopt );
    }
};

class SetColorSwitchLevel : public Overloaded<LuaBinding<void( int, int, std::string, std::string, int )>,
                                              LuaBinding<void( int, int, std::string, std::string )>> {
public:
    void operator()( int nodeId, int channelId, std::string component, std::string command, int duration ) const {
        ZWBND_LOGD(
            "zwave::color_switch_level_change( nodeId=%d, channelId=%d, component=%s, command=%s, duration=%d )",
            nodeId,
            channelId,
            component.c_str(),
            command.c_str(),
            duration );
        return m_ctx->getExecutor().send<::bus::command::ColorSwitchLevelChange>( ZNET_NAME,
                                                                                  nodeId,
                                                                                  channelId,
                                                                                  std::move( command ),
                                                                                  std::move( component ),
                                                                                  std::chrono::seconds{ duration } );
    }

    void operator()( int nodeId, int channelId, std::string component, std::string command ) const {
        ZWBND_LOGD( "zwave::color_switch_level_change( nodeId=%d, channelId=%d, component=%s, command=%s )",
                    nodeId,
                    channelId,
                    component.c_str(),
                    command.c_str() );
        return m_ctx->getExecutor().send<::bus::command::ColorSwitchLevelChange>(
            ZNET_NAME, nodeId, channelId, std::move( command ), std::move( component ), std::nullopt );
    }
};

class StartInclude : public Overloaded<LuaBinding<void( std::string )>, LuaBinding<void()>> {
public:
    void operator()( std::string deviceSettingsFilepath ) const {
        ZWBND_LOGD( "zwave::start_include( deviceSettingsFilepath=%s )", deviceSettingsFilepath.c_str() );
        m_ctx->getExecutor().send(
            ZNET_NAME, std::make_unique<::bus::command::StartInclude>( std::move( deviceSettingsFilepath ) ) );
    }

    void operator()() const {
        ZWBND_LOGD( "zwave::start_include()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::StartInclude>() );
    }
};

class StartExclude : public Overloaded<LuaBinding<bool( std::string )>, LuaBinding<void()>> {
public:
    bool operator()( std::string nodeId ) const {
        ZWBND_LOGD( "zwave::start_exclude( nodeId=%s )", nodeId.c_str() );
        if( nodeId.empty() ) {
            ZWBND_LOGE( "start_exclude failed: the field \"nodeId\" is empty" );
            return false;
        }
        auto bus_command = std::make_unique<::bus::command::StartExclude>();
        bus_command->m_nodeId = std::move( nodeId );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
        return true;
    }

    void operator()() const {
        ZWBND_LOGD( "zwave::start_exclude()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::StartExclude>() );
    }
};

class StartLearn : public LuaBinding<void()> {
public:
    void operator()() const {
        ZWBND_LOGD( "zwave::start_learn()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::StartLearn>() );
    }
};

class StartRediscover : public LuaBinding<void( int, std::string )> {
public:
    void operator()( int node_id, std::string deviceSettingsFilepath ) const {
        ZWBND_LOGD( "zwave::start_rediscover( node_id=%d, deviceSettingsFilepath=%s )",
                    node_id,
                    deviceSettingsFilepath.c_str() );
        m_ctx->getExecutor().send(
            ZNET_NAME,
            std::make_unique<::bus::command::StartRediscover>( node_id, std::move( deviceSettingsFilepath ) ) );
    }
};

class StopInclude : public LuaBinding<void()> {
public:
    void operator()() const {
        ZWBND_LOGD( "zwave::stop_include()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::StopInclude>() );
    }
};

class StopSecurityInclude : public LuaBinding<void()> {
public:
    void operator()() const {
        ZWBND_LOGD( "zwave::stop_security_include()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::StopSecurityInclude>() );
    }
};

class StopExclude : public LuaBinding<void()> {
public:
    void operator()() const {
        ZWBND_LOGD( "zwave::stop_exclude()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::StopExclude>() );
    }
};

class StopLearn : public LuaBinding<void()> {
public:
    void operator()() const {
        ZWBND_LOGD( "zwave::stop_learn()" );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::StopLearn>() );
    }
};

class AddConfiguration : public LuaBinding<void( int, int, int )> {
public:
    void operator()( int nodeId, int argNumber, int argSize ) const {
        ZWBND_LOGD( "zwave::add_configuration(nodeId=%d, argNumber=%d, argSize=%d ", nodeId, argNumber, argSize );
        auto bus_command = std::make_unique<::configuration::commands::AddConfiguration>( nodeId, argNumber, argSize );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class RemoveConfiguration : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int argNumber ) const {
        ZWBND_LOGD( "zwave::remove_configuration(nodeId=%d, argNumber=%d)", nodeId, argNumber );
        auto bus_command = std::make_unique<::configuration::commands::RemoveConfiguration>( nodeId, argNumber );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetProvisioningInfo : public LuaBinding<void( int, std::optional<std::string>, std::optional<std::string> )> {
public:
    void operator()( int nodeId, std::optional<std::string> name, std::optional<std::string> room ) const {
        ZWBND_LOGD( "zwave::set_provisioning_info(nodeId=%d, name=%s, room=%s)",
                    nodeId,
                    name ? name->c_str() : "nil",
                    room ? room->c_str() : "nil" );
        m_ctx->getExecutor().send(
            ZNET_NAME, std::make_unique<gateways::commands::ZwaveProvisioningUpdate>( nodeId, name, room ) );
    }
};

class GetProvisioningInfo : public LuaBinding<std::shared_ptr<::common::TableValue>( int )> {
public:
    std::shared_ptr<::common::TableValue> operator()( int nodeId ) const {
        ZWBND_LOGD( "zwave::get_provisioning_info(nodeId=%d)", nodeId );
        auto response =
            m_ctx->getExecutor().waitToResp<::gateways::commands::ZwaveProvisioningGet>( ZNET_NAME, 5s, nodeId );
        auto returnValue = std::make_shared<::common::TableValue>();
        returnValue->insert( common::resources::lua_events::NodeId, std::make_unique<::common::IntValue>( nodeId ) );
        returnValue->insert( common::resources::lua_events::ProvisioningName,
                             std::make_unique<::common::StringValue>( response.m_name ) );
        returnValue->insert( common::resources::lua_events::ProvisioningRoom,
                             std::make_unique<::common::StringValue>( response.m_room ) );
        return returnValue;
    }
};

class GetFirmwareRegion : public LuaBinding<std::string()> {
public:
    std::string operator()() const {
        ZWBND_LOGD( "zwave::get_firmware_region()" );
        auto& idInfo = core::IDInfo::get();
        const char* region = idInfo.zwaveRegion();
        if( !region ) {
            return "US";
        }

        return region;
    }
};

class SendIdentify : public LuaBinding<void( uint8_t, uint8_t )> {
public:
    void operator()( uint8_t nodeId, uint8_t channelId ) const {
        ZWBND_LOGD( "zwave::send_identify( nodeId=%d, channelId=%d )", nodeId, channelId );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::SendIdentify>( nodeId, channelId ) );
    }
};

class UpdateNodesFirmware : public LuaBinding<void( std::vector<uint8_t>, std::unique_ptr<::common::ArrayValue> )> {
public:
    void operator()( std::vector<uint8_t> node_ids, std::unique_ptr<::common::ArrayValue> firmware ) const {
        auto firmwareJson = firmware->toJson( "firmware" );

        ZWBND_LOGD( "zwave::update_nodes_firmware( node_ids = [...], firmware= ( %s ) )", firmwareJson.c_str() );

        m_ctx->getExecutor().send(
            ZNET_NAME,
            std::make_unique<::bus::command::UpdatesNodesFirmware>( std::move( node_ids ), std::move( firmware ) ) );
    }
};

class SetBarrierSignalValue : public LuaBinding<void( int, int, std::string, int )> {
public:
    void operator()( int nodeId, int channelId, std::string signalName, int value ) const {
        ZWBND_LOGD( "zwave::set_barrier_signal_value(nodeId=%d, channelId=%d, signalName=%s, "
                    "value=0x%X) const",
                    nodeId,
                    channelId,
                    signalName.c_str(),
                    value );
        auto bus_command = std::make_unique<::bus::command::SetBarrierSignal>( nodeId, channelId, signalName, value );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class RequestBarrierSignalValue : public LuaBinding<void( int, int, std::string )> {
public:
    void operator()( int nodeId, int channelId, std::string signalName ) const {
        ZWBND_LOGD( "zwave::request_barrier_signal_value( nodeId=%d, channelId=%d, signalName %s )",
                    nodeId,
                    channelId,
                    signalName.c_str() );
        auto bus_command = std::make_unique<::bus::command::RequestBarrierSignal>( nodeId, channelId, signalName );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class SetDevicesPollIntervals : public LuaBinding<void( bool, int, int, int )> {
public:
    void operator()( bool useFixedWakeUpInterval,
                     int wakeUpInterval,
                     int alwaysOnDevicesInetrval,
                     int attemptsCount ) const {
        ZWBND_LOGD( "zwave::set_devices_poll_intervals( use_fixed_wake_up_interval=%d, wake_up_interval=%d, "
                    "always_on_devices_inetrval=%d, attempts_count=%d  )",
                    useFixedWakeUpInterval,
                    wakeUpInterval,
                    alwaysOnDevicesInetrval,
                    attemptsCount );
        auto bus_command = std::make_unique<::bus::command::SetDevicePollIntervals>(
            useFixedWakeUpInterval, wakeUpInterval, alwaysOnDevicesInetrval, attemptsCount );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class CheckNodeValues : public LuaBinding<void( int, int, std::unique_ptr<::common::ArrayValue> )> {
public:
    void operator()( int nodeId, int requestDelay, std::unique_ptr<::common::ArrayValue> ccToCheck ) const {
        ZWBND_LOGI( "check_node_values (node_id=%d, request_delay=%d, cc_to_check=%s)",
                    nodeId,
                    requestDelay,
                    ccToCheck->toJson().c_str() );
        m_ctx->getExecutor().send( ZNET_NAME,
                                   std::make_unique<::bus::command::CheckNodeValues>(
                                       nodeId, std::chrono::milliseconds( requestDelay ), std::move( ccToCheck ) ) );
    }
};

class CheckReachableStatus : public LuaBinding<void( int )> {
public:
    void operator()( int nodeId ) const {
        ZWBND_LOGI( "check_reachable_status (node_id=%d)", nodeId );
        m_ctx->getExecutor().send( ZNET_NAME, std::make_unique<::bus::command::CheckReachableStatus>( nodeId ) );
    }
};

class CalculateUserCodesChecksum : public LuaBinding<int( int, int, std::unique_ptr<::common::ArrayValue> )> {
public:
    int operator()( int nodeId, int channelId, std::unique_ptr<::common::ArrayValue> userCodeArray ) const {
        ZWBND_LOGI( "zwave::calculate_user_codes_checksum( node_id=%d, channeld_id=%d, user_codes=%s )",
                    nodeId,
                    channelId,
                    userCodeArray->toJson().c_str() );
        namespace lua_res = ::common::resources::lua_events;
        std::vector<::znet_user_code_report_t> znetUserCodeList;
        for( auto& userCodeIter : *userCodeArray ) {
            if( auto userCodeTable = ::common::iValueCast<::common::TableValue>( userCodeIter.get() ) ) {
                auto id = userCodeTable->getInt( lua_res::Id );
                auto modeStr = userCodeTable->getString( lua_res::Mode );
                auto code = userCodeTable->getString( lua_res::Code );
                if( id && modeStr && code ) {
                    if( auto mode = ::znet::user_code_converter::userIdStatusFromString( modeStr->get().c_str() ) ) {
                        znetUserCodeList.push_back(
                            ::znet_user_code_report_t{ static_cast<znet_supported_users_t>( id->get() ),
                                                       static_cast<std::uint8_t>( *mode ),
                                                       static_cast<std::uint8_t>( code->get().size() ),
                                                       reinterpret_cast<const uint8_t*>( code->get().c_str() ) } );
                    }
                }
            }
        }
        return ::calculate_user_codes_checksum( znetUserCodeList.data(), znetUserCodeList.size() );
    }
};

class LockAntiTheft : public LuaBinding<void( int, int, std::string, std::string )> {
public:
    void operator()( int nodeId, int channelId, std::string magicCode, std::string hint ) const {
        ZWBND_LOGD( "zwave::lock_anti_theft( nodeId=%d, channelId=%d, magicCode %s, hint %s )",
                    nodeId,
                    channelId,
                    magicCode.c_str(),
                    hint.c_str() );
        auto bus_command = std::make_unique<::bus::command::LockAntiTheft>( nodeId, channelId, magicCode, hint );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class LockAntiTheftDefault : public LuaBinding<void( int, int )> {
public:
    void operator()( int nodeId, int channelId ) const {
        ZWBND_LOGD( "zwave::lock_anti_theft_default( nodeId=%d, channelId=%d )", nodeId, channelId );
        auto bus_command = std::make_unique<::bus::command::LockAntiTheftDefault>( nodeId, channelId );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class UnlockAntiTheft : public LuaBinding<void( int, int, int, std::string )> {
public:
    void operator()( int nodeId, int channelId, int classId, std::string magicCode ) const {
        ZWBND_LOGD( "zwave::unlock_anti_theft( nodeId=%d, channelId=%d, classId=%d, magicCode %s )",
                    nodeId,
                    channelId,
                    classId,
                    magicCode.c_str() );
        auto bus_command = std::make_unique<::bus::command::UnlockAntiTheft>( nodeId, channelId, classId, magicCode );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class UnlockAntiTheftDefault : public LuaBinding<void( int, int, int, std::string )> {
public:
    void operator()( int nodeId, int channelId, int classId, std::string hint ) const {
        ZWBND_LOGD( "zwave::unlock_anti_theft_default( nodeId=%d, channelId=%d, classId=%d, hint %s )",
                    nodeId,
                    channelId,
                    classId,
                    hint.c_str() );
        auto bus_command = std::make_unique<::bus::command::UnlockAntiTheftDefault>( nodeId, channelId, classId, hint );
        m_ctx->getExecutor().send( ZNET_NAME, std::move( bus_command ) );
    }
};

class HasNode : public LuaBinding<bool( int )> {
public:
    bool operator()( int nodeId ) const {
        ZWBND_LOGD( "zwave::has_node(id = %d)", nodeId );
        auto device = ::gateways::zwave::Gateway::get().getDevice( nodeId );
        if( !device ) {
            REMOTE_LOGE( __FUNCTION__, "Cannot find device with nodeId=%d", nodeId );
            return false;
        }
        return true;
    }
};

class SetDefaultConfigurtionFile : public LuaBinding<void( std::string )> {
public:
    void operator()( std::string value ) const {
        ZWBND_LOGE( "zwave::set_default_device_configurations_file    NEED TO IMPLEMENT" );
    }
};

}  // namespace zwave
}  // namespace bindings
}  // namespace module
}  // namespace lua
