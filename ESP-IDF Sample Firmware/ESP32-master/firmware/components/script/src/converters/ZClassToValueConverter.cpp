#include "converters/ZClassToValueConverter.h"

#include <sstream>

#include <znet/converters/color_switch_converter.hpp>
#include <znet/converters/multilevel_sensor_converter.hpp>
#include <znet/converters/central_scene_attributes_converter.hpp>
#include <znet/converters/barrier_operator_signal_converter.hpp>
#include <znet/converters/configuration_format_converter.hpp>
#include <znet/converters/signed_configuration_parameter_converter.hpp>
#include <znet/converters/user_code.hpp>
#include <znet/converters/door_lock.hpp>
#include <znet/enums/color_component.hpp>
#include <znet/znet_meter.hpp>

#include "IValue.h"
#include "values/TableValue.h"
#include "values/ArrayValue.h"
#include "values/StringValue.h"
#include "values/IntValue.h"
#include "values/Int64Value.hpp"
#include "values/BoolValue.h"

#include "DebugUtils.h"

#include "CommonResources.h"

namespace lua {
namespace module {
namespace converters {
namespace zwave {

ZClassToValueConverter::ZClassToValueConverter(::common::TableValue& value )
: m_value( value ) {}

template <typename T>
void ZClassToValueConverter::addOptionalField( const char* name, znet::utils::optional<T> value ) {
    if( value ) {
        m_value.insert( name, ::common::IValue::create( *value ) );
    }
}

void ZClassToValueConverter::operator()(::znet::ZClass& cc ) {
    FUNC_LOGD( "ZCc0x%02x", cc.getId() );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x20Basic& cc ) {
    FUNC_LOGD( "ZCc0x20Basic" );
    m_value.insert(::common::resources::lua_events::Id, ::common::IValue::create( cc.getId() ) );
    m_value.insert(::common::resources::lua_events::Version, ::common::IValue::create( cc.getVersion() ) );
    m_value.insert(::common::resources::lua_events::IsSupported, ::common::IValue::create( cc.isSupported() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x32Meter& cc ) {
    FUNC_LOGD( "ZCc0x32Meter" );
    auto scaleList = std::make_unique<::common::ArrayValue>();
    for( size_t i = 0; i < cc.getScaleCount(); ++i ) {
        scaleList->push_back(
            std::make_unique<::common::StringValue>( znet::meter::scaleToString( cc.getType(), cc.getScale( i ) ) ) );
    }

    m_value.insert( "scales", std::move( scaleList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );

    const char* typeName = znet::meter::typeToString( cc.getType() );
    m_value.insert( "type", std::make_unique<::common::StringValue>( typeName ? typeName : "" ) );
    m_value.insert( "reset_supported", std::make_unique<::common::IntValue>( cc.isResetSupported() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x33ColorSwitch& cc ) {
    FUNC_LOGD( "ZCc0x33ColorSwitch" );
    ::znet::ColorComponentID colorComponent{ 0 };
    m_value.insert( "component_ids", ::common::IValue::createArray( [&colorComponent]() {
                        if( colorComponent >= ::znet::ColorComponentID::ColorComponentsCount ) {
                            return std::unique_ptr<::common::IValue>{};
                        }
                        auto componentStr = ::znet::color_switch_converter::componentIdToStr( colorComponent );
                        colorComponent =
                            static_cast<::znet::ColorComponentID>( static_cast<uint8_t>( colorComponent ) + 1 );
                        return ::common::IValue::create( componentStr );
                    } ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x30BinarySensor& cc ) {
    FUNC_LOGD( "ZCc0x30BinarySensor" );
    auto sensorTypeList = std::make_unique<::common::ArrayValue>();
    for( const auto& sensorType : cc.getSensorTypes() ) {
        sensorTypeList->push_back( std::make_unique<::common::IntValue>( sensorType ) );
    }

    m_value.insert( common::resources::lua_events::SensorTypes, std::move( sensorTypeList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x31SensorMultilevel& cc ) {
    FUNC_LOGD( "ZCc0x31SensorMultilevel" );
    auto sensorList = std::make_unique<::common::ArrayValue>();
    for( const auto& sensorInfo : cc.getSupportedSensorsInfo() ) {
        auto sensor = std::make_unique<::common::TableValue>();
        sensor->insert( common::resources::lua_events::SensorType,
                        std::make_unique<::common::StringValue>(
                            ::znet::multilevel_sensor_converter::typeToString( sensorInfo.type ) ) );
        sensorList->push_back( std::move( sensor ) );
    }

    m_value.insert( "sensors", std::move( sensorList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x40ThermostatMode& cc ) {
    FUNC_LOGD( "ZCc0x40ThermostatMode" );
    auto modeList = std::make_unique<::common::ArrayValue>();
    for( size_t i = 0; i < cc.getModesCount(); ++i ) {
        modeList->push_back( std::make_unique<::common::IntValue>( static_cast<int>( cc.getMode( i ) ) ) );
    }

    m_value.insert( "modes", std::move( modeList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x43ThermostatSetpoint& cc ) {
    FUNC_LOGD( "ZCc0x43ThermostatSetpoint" );
    auto setpointList = std::make_unique<::common::ArrayValue>();
    for( const auto& setpointDescription : cc.getDescription() ) {
        auto setpoint = std::make_unique<::common::TableValue>();
        setpoint->insert( "type", std::make_unique<::common::IntValue>( setpointDescription.type ) );
        if( cc.getVersion() > 2 ) {
            setpoint->insert( common::resources::lua_events::MinValue,
                              std::make_unique<::common::IntValue>( setpointDescription.minValue ) );
            setpoint->insert( common::resources::lua_events::MinScale,
                              std::make_unique<::common::IntValue>( setpointDescription.minScale ) );

            setpoint->insert( common::resources::lua_events::MaxValue,
                              std::make_unique<::common::IntValue>( setpointDescription.maxValue ) );
            setpoint->insert( common::resources::lua_events::MaxScale,
                              std::make_unique<::common::IntValue>( setpointDescription.maxScale ) );
        }
        setpointList->push_back( std::move( setpoint ) );
    }

    m_value.insert( "setpoints", std::move( setpointList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x44ThermostatFanMode& cc ) {
    FUNC_LOGD( "ZCc0x44ThermostatFanMode" );
    auto modeList = std::make_unique<::common::ArrayValue>();
    for( size_t i = 0; i < cc.getFanModesCount(); ++i ) {
        modeList->push_back( std::make_unique<::common::IntValue>( static_cast<int>( cc.getFanMode( i ) ) ) );
    }

    m_value.insert( "modes", std::move( modeList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x5BCentralScene& cc ) {
    FUNC_LOGD( "ZCc0x5BCentralScene" );
    auto buttonList = std::make_unique<::common::ArrayValue>();
    for( int i = 1; i <= cc.getSceneNum(); ++i ) {
        auto buttonAttributes = std::make_unique<::common::ArrayValue>();
        for( const auto& name : ::znet::central_scenes_attributes_converter::maskToStrings( cc.getAttribute( i ) ) ) {
            buttonAttributes->push_back( std::make_unique<::common::StringValue>( name ) );
        }
        auto button = std::make_unique<::common::TableValue>();
        button->insert( common::resources::lua_events::ButtonId, std::make_unique<::common::IntValue>( i ) );
        button->insert( common::resources::lua_events::ButtonAttributes, std::move( buttonAttributes ) );
        buttonList->push_back( std::move( button ) );
    }
    m_value.insert( "buttons", std::move( buttonList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x63UserCode& cc ) {
    FUNC_LOGD( "ZCc0x63UserCode" );
    using ::common::IValue;
    namespace lua_res = ::common::resources::lua_events;
    m_value.insert( lua_res::Id, IValue::create( cc.getId() ) );
    m_value.insert( lua_res::Version, IValue::create( cc.getVersion() ) );
    m_value.insert( lua_res::SupportedUsersCount, IValue::create( cc.getSupportedUsersCount() ) );
    if( auto capabilities = cc.getCapabilities() ) {
        m_value.insert(
            lua_res::Capabilities,
            IValue::create( { { lua_res::ChecksumSupported, IValue::create( capabilities->isCheckSumSupported ) },
                              { lua_res::MasterCodeSupported, IValue::create( capabilities->isMasterCodeSupported ) },
                              { lua_res::MasterCodeDeactivationSupported,
                                IValue::create( capabilities->isMasterCodeDeactivationSupported ) },
                              { lua_res::SupportedKeys,
                                IValue::create( std::string{ capabilities->supportedKeys.data(),
                                                             capabilities->supportedKeys.size() } ) },
                              { lua_res::SupportedKeypadModes,
                                IValue::create( capabilities->supportedKeypadModes,
                                                ::znet::user_code_converter::keypadModeToString ) },
                              { lua_res::SupportedUserIdStatuses,
                                IValue::create( capabilities->supportedUserIdStatuses,
                                                ::znet::user_code_converter::userIdStatusToString ) } } ) );
    }
}

void ZClassToValueConverter::operator()(::znet::ZCc0x71Notification& cc ) {
    FUNC_LOGD( "ZCc0x71Notification" );
    auto notificationList = std::make_unique<::common::ArrayValue>();
    for( const auto& notificationInfo : cc.getSupportedNotifications() ) {
        if( auto name = getNotificationName( notificationInfo.m_type ) ) {
            auto knownEventList = std::make_unique<::common::ArrayValue>();
            auto unknownEventList = std::make_unique<::common::ArrayValue>();
            for( const auto& eventType : notificationInfo.m_events ) {
                auto eventData = getNotificationEventData( notificationInfo.m_type, eventType );
                if( eventData && cc.getVersion() >= eventData->m_version ) {
                    knownEventList->push_back( std::make_unique<::common::StringValue>( eventData->m_name ) );
                }
                else {
                    unknownEventList->push_back( std::make_unique<::common::IntValue>( eventType ) );
                }
            }

            auto notification = std::make_unique<::common::TableValue>();
            notification->insert( "type", std::make_unique<::common::StringValue>( name ) );

            if( knownEventList->begin() != knownEventList->end() ) {
                notification->insert( common::resources::lua_events::EventTypes, std::move( knownEventList ) );
            }
            if( unknownEventList->begin() != unknownEventList->end() ) {
                notification->insert( common::resources::lua_events::UnknownEventTypes, std::move( unknownEventList ) );
            }

            notificationList->push_back( std::move( notification ) );
        }
    }

    m_value.insert( "notifications", std::move( notificationList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
    m_value.insert( common::resources::lua_events::IsV1Supported,
                    std::make_unique<::common::BoolValue>( cc.isV1Supported() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x9cAlarmSensor& cc ) {
    FUNC_LOGD( "ZCc0x9cAlarmSensor" );
    auto sensorTypeList = std::make_unique<::common::ArrayValue>();
    for( const auto& type : cc.getSupportedTypes() ) {
        sensorTypeList->push_back(
            std::make_unique<::common::StringValue>(::znet::getExtendedAlarmSensorName( type ) ) );
    }

    // ?? TODO check after the lua script has been added
    m_value.insert( common::resources::lua_events::SensorTypes, std::move( sensorTypeList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x79SoundSwitch& cc ) {
    FUNC_LOGD( "ZCc0x79SoundSwitch" );
    auto toneList = std::make_unique<::common::ArrayValue>();
    for( size_t i = 1; i <= cc.getTonesNumber(); ++i ) {
        auto tone = std::make_unique<::common::TableValue>();
        tone->insert( common::resources::lua_events::ToneId, std::make_unique<::common::IntValue>( i ) );
        tone->insert( "duration", std::make_unique<::common::IntValue>( cc.getToneDuration( i ) ) );
        tone->insert( common::resources::lua_events::ToneName,
                      std::make_unique<::common::StringValue>( cc.getToneName( i ) ) );
        toneList->push_back( std::move( tone ) );
    }

    m_value.insert( "tones", std::move( toneList ) );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x4EScheduleEntryLock& cc ) {
    FUNC_LOGD( "ZCc0x4EScheduleEntryLock" );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );
    m_value.insert( common::resources::lua_events::DailyRepeatingSlots,
                    std::make_unique<::common::IntValue>( cc.getDailyRepeatingSlots() ) );
    m_value.insert( common::resources::lua_events::WeekDaySlots,
                    std::make_unique<::common::IntValue>( cc.getWeekDaySlots() ) );
    m_value.insert( common::resources::lua_events::YearDaySlots,
                    std::make_unique<::common::IntValue>( cc.getYearDaySlots() ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x7AFirmwareUpdate& cc ) {
    FUNC_LOGD( "ZCc0x7AFirmwareUpdate" );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );

    auto firmwareIds = std::make_unique<::common::ArrayValue>();
    for( const auto& firmwareId : cc.getFirmwareIds() ) {
        firmwareIds->push_back( std::make_unique<::common::IntValue>( firmwareId ) );
    }
    m_value.insert( "firmware_ids", std::move( firmwareIds ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x86Version& cc ) {
    FUNC_LOGD( "ZCc0x86Version" );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );

    addOptionalField( "library_type", cc.getLibraryType() );
    addOptionalField( "zwave_protocol_version", cc.getZwaveProtocolVersion() );
    addOptionalField( "hardware_version", cc.getHardwareVersion() );

    if( cc.getFirmwaresCount() != 0 ) {
        auto firmwareVersions = std::make_unique<::common::ArrayValue>();
        for( uint8_t index = 0; index != cc.getFirmwaresCount(); ++index ) {
            firmwareVersions->push_back( std::make_unique<::common::StringValue>( cc.getFirmwareVersion( index ) ) );
        }
        m_value.insert( "firmware_versions", std::move( firmwareVersions ) );
    }

    addOptionalField( "sdk_version", cc.getSdkVersion() );

    addOptionalField( "application_framework_api_version", cc.getApplicationFrameworkApiVersion() );
    addOptionalField( "application_framework_api_build_number", cc.getApplicationFrameworkApiBuildNumber() );

    addOptionalField( "host_interface_version", cc.getHostInterfaceVersion() );
    addOptionalField( "host_interface_build_number", cc.getHostInterfaceBuildNumber() );

    addOptionalField( "protocol_version", cc.getProtocolVersion() );
    addOptionalField( "protocol_build_number", cc.getProtocolBuildNumber() );

    addOptionalField( "application_version", cc.getApplicationVersion() );
    addOptionalField( "application_build_number", cc.getApplicationBuildNumber() );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x66BarrierOperator& cc ) {
    FUNC_LOGD( "ZCc0x66BarrierOperator" );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );

    auto signalName = std::make_unique<::common::ArrayValue>();
    for( const auto& signal : cc.getSupportedSignals() ) {
        signalName->push_back(
            std::make_unique<::common::StringValue>(::znet::barrier_operator_signal_converter::signalToString(
                static_cast<znet::BarrierOperatorSignal>( signal ) ) ) );
    }
    m_value.insert( "signals", std::move( signalName ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x70Configuration& cc ) {
    FUNC_LOGD( "ZCc0x70Configuration" );
    m_value.insert( "id", std::make_unique<::common::IntValue>( cc.getId() ) );
    m_value.insert( "version", std::make_unique<::common::IntValue>( cc.getVersion() ) );

    auto parameters = std::make_unique<::common::ArrayValue>();

    cc.forEachV3Parameter( [&parameters]( const ::znet::ZCc0x70Configuration::Parameter& param ) {
        auto pTable = std::make_unique<::common::TableValue>();
        pTable->insert( "id", std::make_unique<::common::IntValue>( param.m_id ) );
        pTable->insert( "format",
                        std::make_unique<::common::StringValue>(
                            ::znet::configuration_format_converter::formatToString( param.m_format ) ) );
        pTable->insert( "size", std::make_unique<::common::IntValue>( param.m_size ) );
        pTable->insert(
            "min_value",
            std::make_unique<::common::Int64Value>(::znet::signed_configuration_parameter_converter::toUiValue(
                param.m_minValue, param.m_size, param.m_format ) ) );
        pTable->insert(
            "max_value",
            std::make_unique<::common::Int64Value>(::znet::signed_configuration_parameter_converter::toUiValue(
                param.m_maxValue, param.m_size, param.m_format ) ) );
        pTable->insert(
            "default_value",
            std::make_unique<::common::Int64Value>(::znet::signed_configuration_parameter_converter::toUiValue(
                param.m_defaultValue, param.m_size, param.m_format ) ) );
        pTable->insert( "name", std::make_unique<::common::StringValue>( param.m_name ) );
        pTable->insert( "info", std::make_unique<::common::StringValue>( param.m_info ) );
        parameters->push_back( std::move( pTable ) );
    } );

    m_value.insert( "parameters", std::move( parameters ) );
}

void ZClassToValueConverter::operator()(::znet::ZCcAntiTheftBase& cc ) {
    FUNC_LOGD( "ZCcAntiTheftBase" );
    using ::common::IValue;
    namespace lua_res = ::common::resources::lua_events;
    m_value.insert( lua_res::Id, IValue::create( cc.getId() ) );
    m_value.insert( lua_res::Version, IValue::create( cc.getVersion() ) );

    if( const auto& lockInfo = cc.getInitialLockInfo() ) {
        auto pTable = std::make_unique<::common::TableValue>();
        pTable->insert( "hint", std::make_unique<::common::StringValue>( lockInfo->getHexHint() ) );
        pTable->insert( "restricted", std::make_unique<::common::BoolValue>( lockInfo->m_isRestricted ) );
        pTable->insert( "manufacturer_id", std::make_unique<::common::IntValue>( lockInfo->m_manufacturerId ) );
        if( lockInfo->m_zwaveAllianceLockingId ) {
            pTable->insert( "zwave_alliance_locking_id",
                            std::make_unique<::common::IntValue>( *lockInfo->m_zwaveAllianceLockingId ) );
        }
        m_value.insert( "initial_lock_info", std::move( pTable ) );
    }
}

void ZClassToValueConverter::operator()(::znet::ZCc0x5DAntiTheft& cc ) {
    ( *this )( static_cast<::znet::ZCcAntiTheftBase&>( cc ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x7EAntiTheftUnlock& cc ) {
    ( *this )( static_cast<::znet::ZCcAntiTheftBase&>( cc ) );
}

void ZClassToValueConverter::operator()(::znet::ZCc0x62DoorLock& cc ) {
    FUNC_LOGD( "ZCc0x62DoorLock" );
    using ::common::IValue;
    namespace lua_res = ::common::resources::lua_events;

    m_value.insert( lua_res::Id, IValue::create( cc.getId() ) );
    m_value.insert( lua_res::Version, IValue::create( cc.getVersion() ) );

    if( const auto capabilities = cc.getCapabilities() ) {
        auto InsideHandleIds = std::make_unique<::common::ArrayValue>();
        ::znet::utils::forEachBit( &capabilities->supportedInsideDoorHandlesBitmask,
                                   sizeof( capabilities->supportedInsideDoorHandlesBitmask ),
                                   [&InsideHandleIds]( uint8_t InsideHandleIdsIndex ) {
                                       InsideHandleIds->push_back( IValue::create( InsideHandleIdsIndex + 1 ) );
                                   } );

        auto OutsideHandleIds = std::make_unique<::common::ArrayValue>();
        ::znet::utils::forEachBit( &capabilities->supportedOutsideDoorHandlesBitmask,
                                   sizeof( capabilities->supportedOutsideDoorHandlesBitmask ),
                                   [&OutsideHandleIds]( uint8_t IOutsideHandleIdsIndex ) {
                                       OutsideHandleIds->push_back( IValue::create( IOutsideHandleIdsIndex + 1 ) );
                                   } );

        m_value.insert(
            lua_res::Capabilities,
            IValue::create(
                { { lua_res::DoorSupported, IValue::create( capabilities->isDoorSupported ) },
                  { lua_res::BoltSupported, IValue::create( capabilities->isBoltSupported ) },
                  { lua_res::LatchSupported, IValue::create( capabilities->isLatchSupported ) },
                  { lua_res::BlockToBlockSupported, IValue::create( capabilities->isBlockToBlockSupported ) },
                  { lua_res::TwistAssistSupported, IValue::create( capabilities->isTwistAssistSupported ) },
                  { lua_res::HoldAndReleaseSupported, IValue::create( capabilities->isHoldAndReleaseSupported ) },
                  { lua_res::TimeOperationsSupported, IValue::create( capabilities->areTimeOperationsSupported ) },
                  { lua_res::AutoRelockSupported, IValue::create( capabilities->isAutoRelockSupported ) },
                  { lua_res::DoorLockModes,
                    IValue::create( capabilities->supportedModes, ::znet::door_lock_converter::modeToString ) },
                  { lua_res::InsideHandleIds, std::move( InsideHandleIds ) },
                  { lua_res::OutsideHandleIds, std::move( OutsideHandleIds ) }

                } ) );
    }
}

}  // namespace zwave
}  // namespace converters
}  // namespace module
}  // namespace lua
