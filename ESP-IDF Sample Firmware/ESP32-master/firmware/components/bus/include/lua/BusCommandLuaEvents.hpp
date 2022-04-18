#pragma once

#include <variant>

#include <znet/znet_utils.hpp>
#include <znet/znet_error_message.hpp>
#include <znet/znet_week_day.hpp>
#include <znet/znet_notification.hpp>
#include <znet/enums/thermostat_setback.hpp>
#include <znet/converters/thermostat_setback_converter.hpp>
#include <znet/converters/user_code.hpp>
#include <znet/anti_theft_info.hpp>
#include <znet/iznet_owner.hpp>

#include "IValue.h"
#include "values/TableValue.h"
#include "values/ArrayValue.h"
#include "DebugUtils.h"

#include "lua/BusCommandGenericLuaEvent.h"
#include "lua/BusCommandSendEventToLuaHandler.hpp"
#include "lua/BusCommandLuaEventsImpl.h"
#include "configuration/ExtDoorLockConfiguration.h"

namespace lua_res = ::common::resources::lua_events;

namespace bus::lua_event {

inline std::unique_ptr<GenericLuaEvent> deviceUpdated( std::string deviceId,
                                                       std::string fieldName,
                                                       std::string fieldValue ) {
    return std::unique_ptr<GenericLuaEvent>(
        new GenericLuaEvent( GenericLuaEvent::DeviceUpdated,
                             { { "_id", ::common::IValue::create( std::move( deviceId ) ) },
                               { std::move( fieldName ), ::common::IValue::create( std::move( fieldValue ) ) } } ) );
}

inline std::unique_ptr<GenericLuaEvent> configurationValueUpdated( uint8_t nodeId,
                                                                   uint8_t channelId,
                                                                   uint8_t classId,
                                                                   uint16_t paramNumber,
                                                                   uint8_t paramSize,
                                                                   int64_t paramValue ) {
    return std::unique_ptr<GenericLuaEvent>(
        new GenericLuaEvent( GenericLuaEvent::ConfigurationValueUpdated,
                             { { "node_id", ::common::IValue::create( nodeId ) },
                               { "channel_id", ::common::IValue::create( channelId ) },
                               { "class_id", ::common::IValue::create( classId ) },
                               { "parameter_num", ::common::IValue::create( paramNumber ) },
                               { "parameter_size", ::common::IValue::create( paramSize ) },
                               { "value", ::common::IValue::create( paramValue ) } } ) );
}

inline std::unique_ptr<GenericLuaEvent> configurationRequestStatusUpdated( uint8_t nodeId,
                                                                           uint8_t paramNumber,
                                                                           std::string status ) {
    return std::unique_ptr<GenericLuaEvent>(
        new GenericLuaEvent( GenericLuaEvent::ConfigurationRequestStatusUpdated,
                             { { "node_id", ::common::IValue::create( nodeId ) },
                               { "parameter_num", ::common::IValue::create( paramNumber ) },
                               { "status", ::common::IValue::create( std::move( status ) ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> moduleResetUpdated( std::string name,
                                                                                         std::string status ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new bus::command::SendGenericEventToLuaHandler(
            name,
            common::resources::broadcasts::events::ModuleReset,
            { { common::resources::lua_events::Status, ::common::IValue::create( std::move( status ) ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> updateState( std::string name,
                                                                                  const std::string& event,
                                                                                  uint8_t node_id,
                                                                                  ::znet::ErrorOpt error ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new bus::command::SendGenericEventToLuaHandler(
            std::move( name ),
            event.c_str(),
            { { common::resources::lua_events::NodeId, ::common::IValue::create( node_id ) },
              { common::resources::lua_events::Error, impl::errorMessageToValue( std::move( error ) ) } } ) );
}

inline std::unique_ptr<bus::command::SendGenericEventToLuaHandler> firmwareUpdateStatus( std::string handlerName,
                                                                                         const uint8_t nodeId,
                                                                                         std::optional<uint16_t> target,
                                                                                         std::string status,
                                                                                         uint8_t progress = {} ) {
    if( target ) {
        return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
            new bus::command::SendGenericEventToLuaHandler(
                handlerName,
                common::resources::broadcasts::events::NodeFirmwareUpdateStatus,
                { { common::resources::lua_events::NodeId, ::common::IValue::create( nodeId ) },
                  { common::resources::lua_events::Target, ::common::IValue::create( *target ) },
                  { common::resources::lua_events::Status, ::common::IValue::create( status ) },
                  { common::resources::lua_events::Progress, ::common::IValue::create( progress ) } } ) );
    }
    else {
        return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
            new bus::command::SendGenericEventToLuaHandler(
                handlerName,
                common::resources::broadcasts::events::NodeFirmwareUpdateStatus,
                { { common::resources::lua_events::NodeId, ::common::IValue::create( nodeId ) },
                  { common::resources::lua_events::Status, ::common::IValue::create( status ) },
                  { common::resources::lua_events::Progress, ::common::IValue::create( progress ) } } ) );
    }
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> firmwareDownloadStatus( std::string handlerName,
                                                                                             std::string image,
                                                                                             std::string checksum,
                                                                                             std::string status,
                                                                                             uint8_t progress ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new bus::command::SendGenericEventToLuaHandler(
            handlerName,
            common::resources::broadcasts::events::NodeFirmwareDownloadStatus,
            { { common::resources::lua_events::Image, ::common::IValue::create( image ) },
              { common::resources::lua_events::Checksum, ::common::IValue::create( checksum ) },
              { common::resources::lua_events::Status, ::common::IValue::create( status ) },
              { common::resources::lua_events::Progress, ::common::IValue::create( progress ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> doorLockConfigurationUpdated(
    std::string handlerName,
    uint8_t nodeId,
    uint8_t channelId,
    std::vector<uint8_t> insideHandleIds,
    std::vector<uint8_t> outsideHandleIds,
    std::optional<uint16_t> lockTimeout,
    std::optional<::configuration::ExtDoorLockConfiguration> extConf ) {
    using ::common::IValue;
    namespace lua_res = ::common::resources::lua_events;
    auto confTable = std::make_unique<::common::TableValue>();
    if( lockTimeout ) {
        confTable->insert( lua_res::LockTimeout, IValue::create( *lockTimeout ) );
    }
    if( extConf ) {
        confTable->insert( lua_res::AutoRelockTimeout, IValue::create( extConf->m_autoRelockTimeout ) );
        confTable->insert( lua_res::HoldAndReleaseTimeout, IValue::create( extConf->m_holdAndReleaseTimeout ) );
        confTable->insert( lua_res::TwistAssistEnabled, IValue::create( extConf->m_twistAssistEnabled ) );
        confTable->insert( lua_res::BlockToBlockEnabled, IValue::create( extConf->m_blockToBlockEnabled ) );
    }
    confTable->insert( lua_res::EnabledInsideHandleIds, IValue::create( std::move( insideHandleIds ) ) );
    confTable->insert( lua_res::EnabledOutsideHandleIds, IValue::create( std::move( outsideHandleIds ) ) );
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            ::common::resources::broadcasts::events::DoorLockConfigurationUpdated,
            { { lua_res::NodeId, ::common::IValue::create( nodeId ) },
              { lua_res::ChannelId, ::common::IValue::create( channelId ) },
              { lua_res::Value, std::move( confTable ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> doorLockStateUpdated(
    std::string handlerName,
    uint8_t nodeId,
    uint8_t channelId,
    std::string doorMode,
    std::string latchState,
    std::string boltState,
    std::string doorState,
    std::vector<uint8_t> insideHandleIds,
    std::vector<uint8_t> outsideHandleIds,
    std::optional<uint16_t> lockTimeout,
    std::optional<std::string> targetMode,
    std::optional<std::string> targetModeTimeout ) {
    namespace lua_res = ::common::resources::lua_events;
    using ::common::IValue;
    auto doorCondition = std::make_unique<::common::TableValue>();
    doorCondition->insert( lua_res::LatchState, IValue::create( latchState ) );
    doorCondition->insert( lua_res::BoltState, IValue::create( boltState ) );
    doorCondition->insert( lua_res::DoorState, IValue::create( doorState ) );
    ::bus::command::SendGenericEventToLuaHandler::table_string_to_ivalue eventFields;
    eventFields.emplace( lua_res::NodeId, IValue::create( nodeId ) );
    eventFields.emplace( lua_res::ChannelId, IValue::create( channelId ) );
    eventFields.emplace( lua_res::DoorLockMode, IValue::create( std::move( doorMode ) ) );
    eventFields.emplace( lua_res::DoorCondition, std::move( doorCondition ) );
    eventFields.emplace( lua_res::EnabledInsideHandleIds, IValue::create( std::move( insideHandleIds ) ) );
    eventFields.emplace( lua_res::EnabledOutsideHandleIds, IValue::create( std::move( outsideHandleIds ) ) );
    if( lockTimeout ) {
        eventFields.emplace( lua_res::LockTimeout, IValue::create( *lockTimeout ) );
    }
    if( targetMode ) {
        eventFields.emplace( lua_res::TargetDoorLockMode, IValue::create( *targetMode ) );
    }
    if( targetModeTimeout ) {
        eventFields.emplace( lua_res::TargetDoorLockModeTimeout, IValue::create( *targetModeTimeout ) );
    }
    return std::make_unique<::bus::command::SendGenericEventToLuaHandler>(
        std::move( handlerName ),
        ::common::resources::broadcasts::events::DoorLockStateUpdated,
        std::move( eventFields ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> updateBarrierSignal( std::string name,
                                                                                          uint8_t node_id,
                                                                                          uint8_t channel_id,
                                                                                          uint8_t class_id,
                                                                                          std::string signal_name,
                                                                                          uint8_t value ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new bus::command::SendGenericEventToLuaHandler(
            name,
            "barrier_signal_value_updated",
            {
                { common::resources::lua_events::NodeId, ::common::IValue::create( node_id ) },
                { common::resources::lua_events::ChannelId, ::common::IValue::create( channel_id ) },
                { common::resources::lua_events::ClassId, ::common::IValue::create( class_id ) },
                { common::resources::lua_events::SignalName, ::common::IValue::create( signal_name ) },
                { common::resources::lua_events::Value, ::common::IValue::create( value ) },
            } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> updateNodeStatus( std::string name,
                                                                                       uint8_t node_id,
                                                                                       std::string status ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new bus::command::SendGenericEventToLuaHandler(
            name,
            "node_check_status",
            { { common::resources::lua_events::NodeId, ::common::IValue::create( node_id ) },
              { common::resources::lua_events::Status, ::common::IValue::create( status ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> nodeRemoved( std::string name,
                                                                                  bool nodeIsKnown,
                                                                                  uint16_t manufacturer_id,
                                                                                  uint16_t product_type,
                                                                                  uint16_t product_id,
                                                                                  uint8_t node_id ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new bus::command::SendGenericEventToLuaHandler(
            name,
            nodeIsKnown ? "node_removed" : "unknown_node_removed",
            { { common::resources::lua_events::NodeId, ::common::IValue::create( node_id ) },
              { common::resources::lua_events::ManufacturerId, ::common::IValue::create( manufacturer_id ) },
              { common::resources::lua_events::ProductType, ::common::IValue::create( product_type ) },
              { common::resources::lua_events::ProductId, ::common::IValue::create( product_id ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> soundSwitchConfiguration( std::string handlerName,
                                                                                               znet_node_id_t nodeId,
                                                                                               uint8_t channelId,
                                                                                               uint8_t volume,
                                                                                               uint8_t defaultToneId ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "sound_switch_configuration_updated",
            { { lua_res::NodeId, ::common::IValue::create( nodeId ) },
              { lua_res::ClassId, ::common::IValue::create( ZNET_COMMAND_CLASS_SOUND_SWITCH ) },
              { lua_res::ChannelId, ::common::IValue::create( channelId ) },
              { lua_res::Volume, ::common::IValue::create( volume ) },
              { lua_res::DefaultToneId, ::common::IValue::create( defaultToneId ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> soundSwitchTonePlay( std::string handlerName,
                                                                                          znet_node_id_t nodeId,
                                                                                          uint8_t channelId,
                                                                                          uint8_t toneId,
                                                                                          uint8_t volume ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "sound_switch_tone_play_updated",
            { { lua_res::NodeId, ::common::IValue::create( nodeId ) },
              { lua_res::ClassId, ::common::IValue::create( ZNET_COMMAND_CLASS_SOUND_SWITCH ) },
              { lua_res::ChannelId, ::common::IValue::create( channelId ) },
              { lua_res::ToneId, ::common::IValue::create( toneId ) },
              { lua_res::Volume, ::common::IValue::create( volume ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> yearDayEntryLockSchedule(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t userId,
    uint8_t slotId,
    const std::optional<znet_year_day_schedule_t>& schedule ) {
    using ::common::IValue;
    return impl::entryLockSchedule(
        "year_day_entry_lock_schedule_updated",
        std::move( handlerName ),
        nodeId,
        channelId,
        userId,
        slotId,
        schedule ? ::common::IValue::create( { { lua_res::StartYear, IValue::create( 2000 + schedule->start_year ) },
                                               { lua_res::StartMonth, IValue::create( schedule->start_month ) },
                                               { lua_res::StartDay, IValue::create( schedule->start_day ) },
                                               { lua_res::StartHour, IValue::create( schedule->start_hour ) },
                                               { lua_res::StartMin, IValue::create( schedule->start_minute ) },
                                               { lua_res::StopYear, IValue::create( 2000 + schedule->stop_year ) },
                                               { lua_res::StopMonth, IValue::create( schedule->stop_month ) },
                                               { lua_res::StopDay, IValue::create( schedule->stop_day ) },
                                               { lua_res::StopHour, IValue::create( schedule->stop_hour ) },
                                               { lua_res::StopMin, IValue::create( schedule->stop_minute ) } } ) :
                   std::unique_ptr<IValue>{} );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> weekDayEntryLockSchedule(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t userId,
    uint8_t slotId,
    const std::optional<znet_week_day_schedule_t>& schedule ) {
    using ::common::IValue;
    std::unique_ptr<IValue> weekDayName;
    if( schedule ) {
        auto weekDayNameStr = ::znet::getWeekDayName( static_cast<::znet::WeekDay>( schedule->day_of_week ) );
        if( !weekDayNameStr ) {
            FUNC_LOGE( "Cannot parse week day name: %d", schedule->day_of_week );
            return nullptr;
        }
        weekDayName = IValue::create( weekDayNameStr );
    }
    return impl::entryLockSchedule(
        "week_day_entry_lock_schedule_updated",
        std::move( handlerName ),
        nodeId,
        channelId,
        userId,
        slotId,
        schedule ? ::common::IValue::create( { { lua_res::WeekDay, std::move( weekDayName ) },
                                               { lua_res::StartHour, IValue::create( schedule->start_hour ) },
                                               { lua_res::StartMin, IValue::create( schedule->start_min ) },
                                               { lua_res::StopHour, IValue::create( schedule->stop_hour ) },
                                               { lua_res::StopMin, IValue::create( schedule->stop_min ) } } ) :
                   std::unique_ptr<IValue>{} );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> dailyRepeatingEntryLockSchedule(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t userId,
    uint8_t slotId,
    const std::optional<znet_daily_repeating_schedule_t>& schedule ) {
    using ::common::IValue;
    std::unique_ptr<::common::ArrayValue> weekDays;
    if( schedule ) {
        weekDays = std::make_unique<::common::ArrayValue>();
        ::znet::utils::forEachBit(
            &schedule->week_day_bitmask, sizeof( schedule->week_day_bitmask ), [&weekDays]( uint8_t weekDayIndex ) {
                if( auto weekDayName = ::znet::getWeekDayName( static_cast<::znet::WeekDay>( weekDayIndex ) ) ) {
                    weekDays->push_back( IValue::create( weekDayName ) );
                }
            } );
    }
    return impl::entryLockSchedule(
        "daily_repeating_entry_lock_schedule_updated",
        std::move( handlerName ),
        nodeId,
        channelId,
        userId,
        slotId,
        schedule ? ::common::IValue::create( { { lua_res::WeekDays, std::move( weekDays ) },
                                               { lua_res::StartHour, IValue::create( schedule->start_hour ) },
                                               { lua_res::StartMin, IValue::create( schedule->start_min ) },
                                               { lua_res::DurationHour, IValue::create( schedule->duration_hour ) },
                                               { lua_res::DurationMin, IValue::create( schedule->duration_min ) } } ) :
                   std::unique_ptr<IValue>{} );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> reachableStateUpdated( std::string handlerName,
                                                                                            znet_node_id_t nodeId,
                                                                                            bool value ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler( std::move( handlerName ),
                                                          "reachable_state_updated",
                                                          { { lua_res::NodeId, ::common::IValue::create( nodeId ) },
                                                            { lua_res::Value, ::common::IValue::create( value ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>
learnProgress( std::string handlerName, znet_node_id_t devicesAmount, znet_node_id_t currentDeviceIndex ) {
    return impl::deviceListOperationProgress(::common::resources::broadcasts::events::LearnProgress,
                                             std::move( handlerName ),

                                             devicesAmount,
                                             currentDeviceIndex );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>
reconstructProgress( std::string handlerName, znet_node_id_t devicesAmount, znet_node_id_t currentDeviceIndex ) {
    return impl::deviceListOperationProgress(::common::resources::broadcasts::events::ReconstructProgress,
                                             std::move( handlerName ),

                                             devicesAmount,
                                             currentDeviceIndex );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> reconstructProgressError( std::string handlerName,
                                                                                               znet_node_id_t nodeId ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            ::common::resources::broadcasts::events::ReconstructProgressError,
            { { lua_res::NodeId, ::common::IValue::create( nodeId ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> rediscoverDeviceProgress( std::string handlerName,
                                                                                               znet_node_id_t nodeId,
                                                                                               uint8_t progress,
                                                                                               std::string status,
                                                                                               ::znet::ErrorOpt err ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            ::common::resources::broadcasts::events::RediscoverDeviceProgress,
            { { lua_res::NodeId, ::common::IValue::create( nodeId ) },
              { lua_res::Progress, ::common::IValue::create( progress ) },
              { lua_res::Status, ::common::IValue::create( std::move( status ) ) },
              { lua_res::Error, impl::errorMessageToValue( std::move( err ) ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> hail( std::string handlerName,
                                                                           znet_node_id_t nodeId,
                                                                           uint8_t commandClassId ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "hail",
            { { lua_res::NodeId, ::common::IValue::create( nodeId ) },
              { lua_res::ClassId, ::common::IValue::create( commandClassId ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> nif( std::string handlerName,
                                                                          znet_node_id_t nodeId,
                                                                          uint8_t commandClassId ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "nif",
            { { lua_res::NodeId, ::common::IValue::create( nodeId ) },
              { lua_res::ClassId, ::common::IValue::create( commandClassId ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> includeDeviceProgress( std::string handlerName,
                                                                                            uint8_t progress ) {
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            ::common::resources::broadcasts::events::IncludeDeviceProgress,
            { { lua_res::Value, ::common::IValue::create( progress ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> valueUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t commandClassId,
    uint8_t value,
    ::znet::utils::optional<uint8_t> targetValue,
    ::znet::utils::optional<std::chrono::seconds> targetDuration ) {
    using ::common::IValue;
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "value_updated",
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::ClassId, IValue::create( commandClassId ) },
              { lua_res::Value, IValue::create( value ) },
              { lua_res::TargetValue, targetValue ? IValue::create( *targetValue ) : std::unique_ptr<IValue>{} },
              { lua_res::TargetDuration,
                targetDuration ? IValue::create( targetDuration->count() * 1000 ) : std::unique_ptr<IValue>{} } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> tokenValueUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t commandClassId,
    std::unique_ptr<::common::IValue> value ) {
    return impl::updateValueEvent( "token_value_updated",
                                   lua_res::Value,
                                   std::move( handlerName ),
                                   nodeId,
                                   channelId,
                                   commandClassId,
                                   std::move( value ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> rgbValueUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    std::unique_ptr<::common::IValue> colors,
    std::unique_ptr<::common::IValue> targetColors,
    std::unique_ptr<::common::IValue> targetDurations ) {
    using ::common::IValue;
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "rgb_value_updated",
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::Colors, std::move( colors ) },
              { lua_res::TargetColors, std::move( targetColors ) },
              { lua_res::TargetDurations, std::move( targetDurations ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> thermostatSetbackValueUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    const ::znet::thermostat_setback::OverrideType overrideType,
    const ::znet::thermostat_setback::Mode mode,
    const ::znet::thermostat_setback::SetbackValueOpt value ) {
    using ::common::IValue;
    ::bus::command::SendGenericEventToLuaHandler::table_string_to_ivalue event;
    event[lua_res::NodeId] = IValue::create( nodeId );
    event[lua_res::ChannelId] = IValue::create( channelId );
    if( value ) {
        event[lua_res::OverrideType] =
            IValue::create(::znet::thermostat_setback::overrideTypeToString( overrideType ) );
        event[lua_res::Mode] = IValue::create(::znet::thermostat_setback::modeToString( mode ) );
        event[lua_res::Value] = IValue::create( *value );
    }
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ), "thermostat_setback_value_updated", std::move( event ) ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> userCodeUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    znet_supported_users_t usersCount,
    const char* userPollingStatus,
    const ::znet::UserCode::UserCodeRanges& userCodes ) {
    using ::common::IValue;
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "user_code_updated",
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::ClassId, IValue::create( ZNET_COMMAND_CLASS_USER_CODE ) },
              { lua_res::SupportedUsersCount, IValue::create( usersCount ) },
              { lua_res::UserPollingStatus, IValue::create( userPollingStatus ) },
              { lua_res::UserCodeRanges, IValue::create( userCodes, []( const ::znet::UserCode::UserCodeRange& range ) {
                    return IValue::create(
                        { { lua_res::UserCode, IValue::create( range.code ) },
                          { lua_res::UserStatus,
                            IValue::create(::znet::user_code_converter::userIdStatusToString( range.status ) ) },
                          { lua_res::UserId, IValue::create( range.id ) },
                          { lua_res::LastUserId,
                            range.lastId ? IValue::create( range.lastId ) : std::unique_ptr<IValue>{} } } );
                } ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> masterCodeUpdated( std::string handlerName,
                                                                                        znet_node_id_t nodeId,
                                                                                        uint8_t channelId,
                                                                                        std::string code ) {
    return impl::userCodeV2Event(
        "master_code_updated", std::move( handlerName ), nodeId, channelId, std::move( code ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>
userCodesKeypadModeUpdated( std::string handlerName, znet_node_id_t nodeId, uint8_t channelId, std::string mode ) {
    return impl::userCodeV2Event(
        "user_codes_keypad_mode_updated", std::move( handlerName ), nodeId, channelId, std::move( mode ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> floatValueUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t classId,
    std::unique_ptr<::common::IValue> type,
    std::unique_ptr<::common::IValue> scale,
    uint8_t precision,
    float value ) {
    return impl::updateFloatValueEvent( "float_value_updated",
                                        lua_res::Type,
                                        std::move( handlerName ),
                                        nodeId,
                                        channelId,
                                        classId,
                                        std::move( type ),
                                        std::move( scale ),
                                        precision,
                                        value );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> thermostatModeValueUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    znet_node_channel_id_t channelId,
    uint8_t mode,
    std::optional<std::string> data ) {
    using ::common::IValue;
    return std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "thermostat_mode_updated",
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::ClassId, IValue::create( ZNET_COMMAND_CLASS_THERMOSTAT_MODE ) },
              { lua_res::Mode, IValue::create( mode ) },
              { lua_res::ManufacturerSpecificData, data ? IValue::create( *data ) : std::unique_ptr<IValue>{} } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> thermostatSetpointValueUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    std::unique_ptr<::common::IValue> type,
    std::unique_ptr<::common::IValue> scale,
    float value,
    bool isSetpointRequested ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "thermostat_setpoint_value_updated",
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ClassId, IValue::create( ZNET_COMMAND_CLASS_THERMOSTAT_SETPOINT ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::Scale, std::move( scale ) },
              { lua_res::Value, IValue::create( value ) },
              { lua_res::Precision, std::unique_ptr<IValue>{} },
              { lua_res::Type, std::move( type ) },
              { lua_res::IsRequested, IValue::create( isSetpointRequested ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> multilevelSensorValueUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    uint8_t classId,
    std::unique_ptr<::common::IValue> type,
    std::unique_ptr<::common::IValue> scale,
    float value ) {
    return impl::updateFloatValueEvent( "multilevel_sensor_value_updated",
                                        lua_res::SensorType,
                                        std::move( handlerName ),
                                        nodeId,
                                        channelId,
                                        classId,
                                        std::move( type ),
                                        std::move( scale ),
                                        std::nullopt,
                                        value );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> alarmSensorValueUpdated( std::string handlerName,
                                                                                              znet_node_id_t nodeId,
                                                                                              uint8_t channelId,
                                                                                              std::string sensorType,
                                                                                              uint16_t delay,
                                                                                              uint8_t value ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "alarm_sensor_value_updated",
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::ClassId, IValue::create( ZNET_COMMAND_CLASS_SENSOR_ALARM ) },
              { lua_res::SensorType, IValue::create( std::move( sensorType ) ) },
              { lua_res::Delay, IValue::create( delay ) },
              { lua_res::Value, IValue::create( value ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> binarySensorValueUpdated( std::string handlerName,
                                                                                               znet_node_id_t nodeId,
                                                                                               uint8_t channelId,
                                                                                               uint8_t ccId,
                                                                                               uint8_t sensorType,
                                                                                               uint8_t value ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler( std::move( handlerName ),
                                                          "binary_sensor_value_updated",
                                                          { { lua_res::NodeId, IValue::create( nodeId ) },
                                                            { lua_res::ChannelId, IValue::create( channelId ) },
                                                            { lua_res::ClassId, IValue::create( ccId ) },
                                                            { lua_res::SensorType, IValue::create( sensorType ) },
                                                            { lua_res::Value, IValue::create( value ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> buttonStateUpdated( std::string handlerName,
                                                                                         znet_node_id_t nodeId,
                                                                                         uint8_t channelId,
                                                                                         uint8_t ccId,
                                                                                         uint8_t sceneNumber,
                                                                                         std::string keyAtribute ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "button_state_updated",
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::ClassId, IValue::create( ccId ) },
              { lua_res::ButtonId, IValue::create( sceneNumber ) },
              { lua_res::Value, IValue::create( std::move( keyAtribute ) ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> sceneActivated( std::string handlerName,
                                                                                     znet_node_id_t nodeId,
                                                                                     uint8_t channelId,
                                                                                     uint8_t ccId,
                                                                                     uint8_t sceneId ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler( std::move( handlerName ),
                                                          "scene_activated",
                                                          { { lua_res::NodeId, IValue::create( nodeId ) },
                                                            { lua_res::ChannelId, IValue::create( channelId ) },
                                                            { lua_res::ClassId, IValue::create( ccId ) },
                                                            { lua_res::SceneId, IValue::create( sceneId ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> s2SelectAuthenticationModes(
    std::string handlerName,
    std::vector<const char*> keyList ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            ::common::resources::broadcasts::events::S2SelectAuthenticationModes,
            { { lua_res::Modes, IValue::create( std::move( keyList ) ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> s2PublicKeyReceived( std::string handlerName,
                                                                                          std::vector<uint16_t> key,
                                                                                          const char* eventName ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ), eventName, { { lua_res::Key, IValue::create( std::move( key ) ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> notificationReceived(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    const char* notificationType,
    uint8_t v1AlarmType,
    uint8_t v1AlarmLevel,
    const char* eventType,
    const char* rawBase64Value,
    ::znet::NotificationValueOpt value ) {
    using ::common::IValue;
    ::bus::command::SendGenericEventToLuaHandler::table_string_to_ivalue event;
    event[lua_res::NodeId] = IValue::create( nodeId );
    event[lua_res::ChannelId] = IValue::create( channelId );
    event[lua_res::ClassId] = IValue::create( ZNET_COMMAND_CLASS_NOTIFICATION );
    event[lua_res::NotificationType] = IValue::create( notificationType );
    event[lua_res::V1AlarmType] = IValue::create( v1AlarmType );
    event[lua_res::V1AlarmLevel] = IValue::create( v1AlarmLevel );
    event[lua_res::EventType] = IValue::create( eventType );
    event[lua_res::RawBase64Value] = IValue::create( rawBase64Value );
    if( value ) {
        znet::visitors::visit(
            znet::visitors::makeOverloads(
                [&event]( const uint8_t val ) { event[lua_res::Value] = IValue::create( val ); },
                [&event]( const std::string& val ) { event[lua_res::Value] = IValue::create( val ); },
                [&event]( const znet::NotificationTimeValue& val ) {
                    event[lua_res::Hours] = IValue::create( val.m_hours );
                    event[lua_res::Minutes] = IValue::create( val.m_minutes );
                    event[lua_res::Seconds] = IValue::create( val.m_seconds );
                } ),
            *value );
    }
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ), "notification_recieved", std::move( event ) ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> unknownNotificationReceived(
    std::string handlerName,
    znet_node_id_t nodeId,
    uint8_t channelId,
    const char* notificationType,
    uint8_t v1AlarmType,
    uint8_t v1AlarmLevel,
    ::znet::NotificationEvent eventType ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler(
            std::move( handlerName ),
            "unknown_notification_recieved",
            { { lua_res::NodeId, IValue::create( nodeId ) },
              { lua_res::ChannelId, IValue::create( channelId ) },
              { lua_res::ClassId, IValue::create( ZNET_COMMAND_CLASS_NOTIFICATION ) },
              { lua_res::NotificationType, IValue::create( notificationType ) },
              { lua_res::V1AlarmType, IValue::create( v1AlarmType ) },
              { lua_res::V1AlarmLevel, IValue::create( v1AlarmLevel ) },
              { lua_res::EventType, IValue::create( eventType ) } } ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> sisUpdate( std::string handlerName ) {
    using ::common::IValue;
    return std::unique_ptr<bus::command::SendGenericEventToLuaHandler>(
        new ::bus::command::SendGenericEventToLuaHandler( std::move( handlerName ), "sis_node_updated", {} ) );
}

inline std::unique_ptr<::bus::command::SendGenericEventToLuaHandler> antiTheftStateUpdated(
    std::string handlerName,
    znet_node_id_t nodeId,
    znet_node_channel_id_t channelId,
    znet_command_class_t commandClassId,
    const ::znet::AntiTheftInfoOpt& antiTheftInfo ) {
    using ::common::IValue;

    ::bus::command::SendGenericEventToLuaHandler::table_string_to_ivalue eventFields;
    eventFields.emplace( lua_res::NodeId, IValue::create( nodeId ) );
    eventFields.emplace( lua_res::ChannelId, IValue::create( channelId ) );
    eventFields.emplace( lua_res::ClassId, IValue::create( commandClassId ) );
    if( antiTheftInfo ) {
        eventFields.emplace(
            lua_res::LockInfo,
            IValue::create( { { lua_res::Restricted, IValue::create( antiTheftInfo->m_isRestricted ) },
                              { lua_res::Hint, IValue::create( antiTheftInfo->getHexHint() ) },
                              { lua_res::ManufacturerId, IValue::create( antiTheftInfo->m_manufacturerId ) },
                              { lua_res::ZwaveAllianceLockingId,
                                antiTheftInfo->m_zwaveAllianceLockingId ?
                                    IValue::create( *( antiTheftInfo->m_zwaveAllianceLockingId ) ) :
                                    std::unique_ptr<IValue>{} } } ) );
    }
    return std::make_unique<::bus::command::SendGenericEventToLuaHandler>(
        std::move( handlerName ), "anti_theft_state_updated", std::move( eventFields ) );
}

}  // namespace bus::lua_event
