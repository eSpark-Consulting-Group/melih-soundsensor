#include <cassert>
#include <cstring>

#include <mbedtls/base64.h>
#include "anti_theft_helper.h"

#include <GatewayID.h>
#include <ZWaveGateway.h>
#include <ZWaveZNet.h>
#include <ZWaveDevicesStorage.h>
#include <ZWaveDeviceSetting.h>
#include <Storage.h>
#include <StorageFile.h>
#include <BinaryFile.h>
#include <GatewayDeviceRestore.h>
#include <NotificationEvent.h>
#include "plugin_manager.h"
#include "DebugUtils.h"
#include "EspLogTags.h"
#include "zwave_device_info.h"

#include <values/IntValue.h>
#include <values/Int64Value.hpp>
#include <values/BoolValue.h>
#include <values/FloatValue.h>
#include <values/RgbValue.h>
#include <values/ScalebleValue.h>
#include <values/StringValue.h>
#include <values/NotificationValue.h>
#include <values/TableValue.h>
#include <values/ArrayValue.h>
#include <IValueCast.h>

#include <znet/cc_0x26_switch_multi_level.hpp>
#include <znet/cc_0x31_sensor_multilevel.hpp>
#include <znet/cc_0x33_color_switch.hpp>
#include <znet/cc_0x40_thermostat_mode.hpp>
#include <znet/cc_0x43_thermostat_setpoint.hpp>
#include "znet/cc_0x47_thermostat_setback.hpp"
#include <znet/cc_0x4e_schedule_entry_lock.hpp>
#include <znet/cc_0x5b_central_scene.hpp>
#include <znet/cc_0x5d_anti_theft.hpp>
#include <znet/cc_0x62_door_lock.hpp>
#include <znet/cc_0x63_user_code.hpp>
#include <znet/cc_0x66_barrier_operator.hpp>
#include <znet/cc_0x70_configuration.hpp>
#include <znet/cc_0x71_notification.hpp>
#include <znet/cc_0x79_sound_switch.hpp>
#include <znet/cc_0x7e_anti_theft_unlock.hpp>
#include <znet/cc_0x87_indicator.hpp>
#include <znet/converters/value_change_command_converter.hpp>
#include <znet/converters/thermostat_setpoint_converter.hpp>
#include <znet/converters/door_lock.hpp>
#include <znet/converters/user_code.hpp>
#include <znet/converters/multilevel_sensor_converter.hpp>
#include <znet/converters/barrier_operator_signal_converter.hpp>
#include <znet/converters/thermostat_setback_converter.hpp>
#include <znet/converters/signed_configuration_parameter_converter.hpp>
#include <znet/converters/mode_status_converter.hpp>
#include <znet/converters/security_modes_converter.hpp>
#include <znet/znet_meter.hpp>
#include <znet/znet_request_value.hpp>
#include <znet/znet_week_day.hpp>
#include <znet/znet_command_class_values_setter.hpp>
#include <znet/znet_utils.hpp>
#include <znet/znet_provisioning_list_query.hpp>
#include <znet/znet_provisioning_list_query_executor.hpp>
#include <znet/enums/configuration_format.hpp>

#include <BusCommandStartInclude.h>
#include <BusCommandS2SelectKey.h>
#include <BusCommandS2SetPublicKey.h>
#include <BusCommandSetHandler.h>
#include <BusCommandHandleEvent.h>
#include <BusCommandUpdateGateway.h>
#include "lua/responce/RemoveDevice.h"
#include <BusCommandSetValue.h>
#include <BusCommandSetValueMulticast.h>
#include "BusCommandSetThermostatMode.hpp"
#include <BusCommandSetUserCode.h>
#include <BusCommandScheduleEntryLock.hpp>
#include <BusCommandStartInclude.h>
#include <BusCommandRequestValue.h>
#include <BusCommandRequestValueTermostatSetpoint.h>
#include <BusCommandRequestUserCode.h>
#include <other_commands/BusCommandRequestAlarmV1Value.hpp>
#include <BusCommandUserCodeV2.hpp>
#include <BusCommandRequestDoorLockState.hpp>
#include <BusCommandSwitchLevelChange.hpp>
#include <BusCommandStartExclude.h>
#include <BusCommandStartRediscover.h>
#include <BusCommandPluginRun.h>
#include <BusCommandSoundSwitch.hpp>
#include <BusCommandGpioButton.hpp>
#include <BusCommandSetBarrierSignal.h>
#include <BusCommandRequestBarrierSignal.h>
#include <BusCommandSetDevicePollIntervals.h>
#include <BusCommandLockAntiTheft.h>
#include <BusCommandLockAntiTheftDefault.h>
#include <BusCommandUnlockAntiTheft.h>
#include <BusCommandUnlockAntiTheftDefault.h>
#include <gateways/commands/BusCommandZwaveProvisioningAdd.h>
#include <gateways/commands/BusCommandZwaveProvisioningRemove.h>
#include <gateways/commands/BusCommandZwaveProvisioningList.h>
#include <gateways/commands/BusCommandZwaveProvisioningUpdate.h>
#include <gateways/commands/BusCommandZwaveProvisioningGet.h>
#include <gateways/commands/BusCommandProvisioningListEntryAdded.hpp>
#include <gateways/commands/BusCommandProvisioningListEntryUpdated.hpp>
#include <gateways/commands/BusCommandProvisioningListEntryRemoved.hpp>
#include <gateways/commands/BusCommandSendIdentify.hpp>
#include <configuration/commands/BusCommandAddConfiguration.h>
#include <configuration/commands/BusCommandSetConfiguration.h>
#include <configuration/commands/BusCommandRequestConfiguration.h>
#include <configuration/commands/BusCommandRemoveConfiguration.h>
#include <configuration/commands/BusCommandSetDoorLockConfiguration.hpp>
#include <configuration/commands/BusCommandRequestDoorLockConfiguration.hpp>
#include <gateways/commands/BusCommandUpdatesNodesFirmware.hpp>
#include <gateways/commands/BusCommandCheckNodeValues.hpp>
#include <gateways/commands/BusCommandCheckReachableStatus.hpp>
#include <configuration/commands/BusCommandResetAllConfigurationValues.hpp>
#include <other_commands/BusCommandRestoreChipBackup.h>
#include <other_commands/BusCommandCreateChipBackup.h>

#include <lua/BusCommandLuaEvents.hpp>
#include <lua/BusCommandSendEventToLuaHandler.hpp>

#include <znet/znet_command_class_values_setter.hpp>
#include <znet/converters/security_modes_converter.hpp>
#include <znet/enums/thermostat_setback.hpp>

#include "BinaryFileStorageFactory.h"
#include "ProvisioningListStorage.h"

#include "FileSource.h"

#include "EspLogTags.h"
#include <IDInfo.h>
#include <LedControl.h>
#include <ResetZW.h>
#include <map>
#include <fstream>

#define YEAR_DAY_LOGD( ... ) ESP_LOGD(::esp_log_tags::ZNET_YEAR_DAY_SCHEDULE, __VA_ARGS__ )
#define YEAR_DAY_LOGE( ... ) ESP_LOGE(::esp_log_tags::ZNET_YEAR_DAY_SCHEDULE, __VA_ARGS__ )
#define WEEK_DAY_LOGD( ... ) ESP_LOGD(::esp_log_tags::ZNET_WEEK_DAY_SCHEDULE, __VA_ARGS__ )
#define WEEK_DAY_LOGE( ... ) ESP_LOGE(::esp_log_tags::ZNET_WEEK_DAY_SCHEDULE, __VA_ARGS__ )
#define DAILY_LOGD( ... ) ESP_LOGD(::esp_log_tags::ZNET_DAILY_REPEATING_SCHEDULE, __VA_ARGS__ )
#define DAILY_LOGE( ... ) ESP_LOGE(::esp_log_tags::ZNET_DAILY_REPEATING_SCHEDULE, __VA_ARGS__ )

//#define SIMPLE_ZWAVE_DEBUG

#define ZWAVE_LOGE( ... ) ESP_LOGE(::esp_log_tags::ZWAVE, __VA_ARGS__ )

#ifdef SIMPLE_ZWAVE_DEBUG

#include <esp_log.h>
#include "EspLogTags.h"

#define ZWAVE_LOGD( ... ) ESP_LOGD(::esp_log_tags::LUA, __VA_ARGS__ )
#define ZWAVE_LOGI( ... ) ESP_LOGI(::esp_log_tags::LUA, __VA_ARGS__ )
#define ZWAVE_LOGW( ... ) ESP_LOGW(::esp_log_tags::LUA, __VA_ARGS__ )

#else

#define ZWAVE_LOGD( ... ) \
    do {                  \
    } while( false )
#define ZWAVE_LOGI( ... ) \
    do {                  \
    } while( false )
#define ZWAVE_LOGW( ... ) \
    do {                  \
    } while( false )

#endif

namespace fields = ::znet::api::filters::fields;

namespace gateways {
namespace zwave {
constexpr const char* ZNet::broadcasts[5][7];

ZNet::ZNet(::znet::IProvisioningListProvider& classicInclusionSecurityStorage,
           ::znet::IProvisioningListProvider& provisioningListStorage )
: ::znet::ZNet( classicInclusionSecurityStorage, provisioningListStorage, *this )
, m_allDevicesRemoved( false ) {
    m_deviceFirmwareUpdateManager = std::make_unique<common::DeviceFirmwareUpdateManager>( *this );
}

ZNet& ZNet::get() {
    static ProvisioningListStorage provisioningListStorage{ "zwave_provisioning_list",
                                                            ::storage::BinaryFileStorageFactory::get() };
    static ProvisioningListStorage classicInclusionStorage{ "classic_inclusion_security_list",
                                                            ::storage::BinaryFileStorageFactory::get() };
    static ZNet znet{ classicInclusionStorage, provisioningListStorage };
    return znet;
}

std::string ZNet::getUpdateImageName() {
    const auto link = m_deviceFirmwareUpdateManager.get()->getActiveUpdateLink();
    if( link ) {
        auto imageUrl = link->image;
        if( !imageUrl.empty() ) {
            const auto image = imageUrl.substr( imageUrl.find_last_of( "/\\" ) + 1 );
            return image;
        }
    }

    return "";
}

void ZNet::startup( gateways::IGateway::ScriptExecutor scriptExecutor ) {
    auto& pluginManager = plugin::PluginManager::get();
    auto pConf = pluginManager.getPluginConfig( "zwave" );
    {
        const char* name;
        name = pConf.json( "initialize_migration" ).toString();
        if( name ) {
            auto script_name = std::string( "zwave/" ) + name + std::string( ".lua" );
            scriptExecutor( script_name );
        }
        ZWAVE_LOGD( "migration started" );
        name = pConf.json( "migration" ).toString();
        if( name ) {
            auto script_name = std::string( "zwave/" ) + name + std::string( ".lua" );
            common::IValue::UPtr res;
            bool isScriptExec;
            bool isFinished = true;
            while( isFinished ) {
                std::tie( isScriptExec, res ) = scriptExecutor( script_name );

                auto retVal = common::iValueCast<common::BoolValue>( std::move( res ) );
                if( retVal ) {
                    ZWAVE_LOGD( "migration in progress retVal %d", retVal->get() );
                    isFinished = retVal->get();
                }
                else {
                    isFinished = false;
                }
            }
        }
        name = pConf.json( "finalize_migration" ).toString();
        if( name ) {
            auto script_name = std::string( "zwave/" ) + name + std::string( ".lua" );
            scriptExecutor( script_name );
        }
        ZWAVE_LOGD( "migration finished" );
    }
    {
        const char* name = pConf.json( "startup" ).toString();
        if( name ) {
            auto script_name = std::string( "zwave/" ) + name + std::string( ".lua" );
            scriptExecutor( script_name );
        }
    }
}

bool ZNet::onLoop() {
    return znet::ZNet::onLoop() && tryReceiveSingleMessage();
}

void ZNet::onUpdateState(::znet::IZNet::Mode cmd, ::znet::ModeStatus cmdStatus, ::znet::ErrorOpt error ) {
    if( cmdStatus == znet::ModeStatus::Busy ) {
        REMOTE_LOGW( __FUNCTION__, "The gateway is busy" );
        send(
            "nma",
            std::make_unique<::bus::command::UpdateGateway>(
                ::gateways::GatewayID::ZWave, ::common::resources::tokens::Ready, ::common::IValue::create( false ) ) );
        return;
    }

    const char* methodName = nullptr;
    switch( cmd ) {
    case IZNet::Mode::IncludeStarting:
    case IZNet::Mode::IncludeStopping:
        methodName = broadcasts[0][(int)cmdStatus];
        break;

    case IZNet::Mode::ExcludeStarting:
    case IZNet::Mode::ExcludeStopping:
        methodName = broadcasts[1][(int)cmdStatus];
        break;

    case IZNet::Mode::LearnStarting:
    case IZNet::Mode::LearnStopping:
        methodName = broadcasts[2][(int)cmdStatus];
        break;

    case IZNet::Mode::NifSending:
        methodName = common::resources::tokens::SendNif;
        break;

    case IZNet::Mode::Initializing:
        if( cmdStatus == znet::ModeStatus::Finished || cmdStatus == znet::ModeStatus::Failed ) {
            if( !::bus::Bus::get().join(::bus::command::GPIO_BUTTON_BUS_CLIENT_NAME, *this ) ) {
                ZWAVE_LOGE( "Cannot join to GpioButton's bus namespace." );
            }
            else if( bus::Bus::get().join( BUS_CLIENT_NAME, *this ) ) {
                REMOTE_LOGD( __FUNCTION__, "znet bus's client has been joined" );
                m_isZnetInitCompleted = true;
            }
        }
        return;

    case znet::IZNet::Mode::BackupCreation:
        ZWAVE_LOGD( "znet::IZNet::Mode::BackupCreation" );
        if( cmdStatus == znet::ModeStatus::Finished || cmdStatus == znet::ModeStatus::Failed ) {
            methodName = broadcasts[4][(int)cmdStatus];
            send( "nma", std::make_unique<::bus::command::CreateChipBackup>( cmdStatus ) );
        }
        break;
    case znet::IZNet::Mode::BackupRestoring:
        ZWAVE_LOGD( "znet::IZNet::Mode::BackupRestoring" );
        if( cmdStatus == znet::ModeStatus::Finished || cmdStatus == znet::ModeStatus::Failed ) {
            methodName = broadcasts[4][(int)cmdStatus];
            send( "nma", std::make_unique<::bus::command::RestoreChipBackup>( cmdStatus ) );
        }

    case IZNet::Mode::NodeLearning:
        /// INFO: clean storage by callback "reset storage"
        break;

    case IZNet::Mode::Reseting: {
        m_allDevicesRemoved = true;

        notify( [&]( std::string subscriber ) {
            return ::bus::lua_event::moduleResetUpdated( subscriber, broadcasts[4][(int)cmdStatus] );
        } );
    }
        return;

    case IZNet::Mode::NodeListGetting:
        break;

    case IZNet::Mode::ValuesGetting:
        break;

    case IZNet::Mode::RequestNeighbor:
        methodName = broadcasts[3][(int)cmdStatus];
        break;

    default:
        break;
    }

    if( isSubscribersEmpty() || methodName == nullptr ) {
        return;
    }

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::updateState( subscriber, methodName, ZNET_NODE_ID_ANY, error );
    } );
}

znet::ZNode* ZNet::onFindNode( znet_node_id_t nodeId ) {
    if( nodeId < 1 ) {
        return nullptr;
    }
    auto devicePtr = Gateway::get().getDevice( nodeId );
    if( !devicePtr ) {
        return nullptr;
    }
    return devicePtr.get();
}

/**
 * Handles znet log records.
 * Since the library pushes logs by chunks, we collect them in a buffer until we get a CRLF as a delimiter.
 */
void ZNet::znetLog( int lvl, const char* fmt, va_list list, void* arg ) {
    static constexpr ::esp_log_level_t level[] = { ESP_LOG_NONE, ESP_LOG_ERROR, ESP_LOG_WARN,
                                                   ESP_LOG_INFO, ESP_LOG_DEBUG, ESP_LOG_VERBOSE };
    static std::array<char, 1024> buffer;
    static char* currentIter = buffer.begin();
    static auto spaceLeft = buffer.size();

    ::vsnprintf( currentIter, spaceLeft, fmt, list );

    if(::strchr( buffer.data(), '\r' ) || ::strchr( buffer.data(), '\n' ) ) {
        char* tokenState = nullptr;
        char* token = ::strtok_r( buffer.data(), "\r\n", &tokenState );
        while( token ) {
            ESP_LOG_LEVEL( level[lvl], ::esp_log_tags::ZWAVE, "%s", token );
            token = ::strtok_r( NULL, "\r\n", &tokenState );
        }
        currentIter = buffer.begin();
        spaceLeft = buffer.size();
    }
    else {
        auto charsWritten = ::vsnprintf( currentIter, spaceLeft, fmt, list );
        spaceLeft -= charsWritten;
        currentIter += charsWritten;
    }
}

void* ZNet::znetAlloc( void* ptr, size_t size, void* arg ) {
    (void)arg;
    return std::realloc( ptr, size );
}

uint32_t ZNet::znetClock( void* arg ) {
    (void)arg;
    return xTaskGetTickCount() * 1000UL / configTICK_RATE_HZ;
}

bool ZNet::uartInit() {
    ZWAVE_LOGI( "UART init requested" );
    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;

    if( uart_driver_install(
            (uart_port_t)CONFIG_UART_PORT, UartBufSize + UART_FIFO_LEN, UartBufSize + UART_FIFO_LEN, 0, NULL, 0 ) !=
        ESP_OK ) {
        return false;
    }
    if( uart_param_config( (uart_port_t)CONFIG_UART_PORT, &uart_config ) != ESP_OK ) {
        return false;
    }
    if( uart_set_pin(
            (uart_port_t)CONFIG_UART_PORT, CONFIG_IO_TX, CONFIG_IO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ) !=
        ESP_OK ) {
        return false;
    }

    return true;
}

void ZNet::uartClose() {
    ZWAVE_LOGI( "UART close requested" );
    if(::uart_is_driver_installed( (uart_port_t)CONFIG_UART_PORT ) ) {
        ZWAVE_LOGI( "Removing UART driver from port %d", CONFIG_UART_PORT );
        if( auto retval = ::uart_driver_delete( (uart_port_t)CONFIG_UART_PORT ); retval != ESP_OK ) {
            ZWAVE_LOGE( "Operation failed, uart_driver_delete returned %d", retval );
            return;
        }
    }
}

int ZNet::znetUartWrite( const void* data, size_t size, size_t* ret_size ) {
    if( !data ) {
        if( !ret_size ) {
            ZWAVE_LOGD( "uart write: invalid args!" );
            return -1;
        }

        *ret_size = UartBufSize;
        return 0;
    }

    if( !size ) {
        ZWAVE_LOGD( "uart write: invalid args!" );
        return -1;
    }
    // esp_log_buffer_hexdump_internal( __func__, data, size, ESP_LOG_INFO );
    int writeLen = uart_write_bytes( (uart_port_t)CONFIG_UART_PORT, (const char*)data, size );
    if( writeLen < 0 )
        return -1;

    if( ret_size )
        *ret_size = writeLen;

    return 0;
}

int ZNet::znetUartRead( void* data, size_t size, size_t* ret_size ) {
    if( !data ) {
        if( !ret_size ) {
            ZWAVE_LOGD( "uart read: invalid args!" );
            return -1;
        }

        if( uart_get_buffered_data_len( (uart_port_t)CONFIG_UART_PORT, ret_size ) != ESP_OK ) {
            *ret_size = 0;
            return -1;
        }

        return 0;
    }

    if( !size ) {
        ZWAVE_LOGD( "uart read: invalid args!" );
        return -1;
    }

    int readLen = uart_read_bytes( (uart_port_t)CONFIG_UART_PORT, (uint8_t*)data, size, 0 /*no wait*/ );
    if( readLen < 0 )
        return -1;

    // esp_log_buffer_hexdump_internal( __func__, data, readLen, ESP_LOG_INFO );
    if( ret_size )
        *ret_size = readLen;

    return 0;
}

int ZNet::znetStoreSave( size_t offset, const void* data, size_t size ) {
    if( !data || !size ) {
        ZWAVE_LOGD( "store save: invalid args!" );
        return -1;
    }
    auto file = storage::BinaryFile::open( STORE_PATH, true );
    if( !file ) {
        ZWAVE_LOGE( "open store for write fail!" );
        return -1;
    }
    if( !file->seek( offset ) ) {
        ZWAVE_LOGE( "seek store for write fail!" );
        return -1;
    }
    if( file->write( data, size ) != size ) {
        ZWAVE_LOGE( "write block to store fail!" );
        return -1;
    }
    return 0;
}

int ZNet::znetStoreLoad( size_t offset, void* data, size_t buffLen ) {
    if( !data || !buffLen ) {
        ZWAVE_LOGD( "store load: invalid args!" );
        return -1;
    }
    auto file = storage::BinaryFile::open( STORE_PATH, false );
    if( !file ) {
        ZWAVE_LOGE( "open store for read fail!" );
        return -1;
    }
    if( !file->seek( offset ) ) {
        ZWAVE_LOGE( "seek store for read fail!" );
        return -1;
    }
    if( auto bytesRead = file->read( data, buffLen ); bytesRead != buffLen ) {
        if( bytesRead >= 0 ) {
            memset( data, 0, buffLen );
            return 0;
        }
        ZWAVE_LOGE( "read block from store fail!" );
        return -1;
    }
    return 0;
}

int ZNet::znetStoreReset() {
    storage::File file;
    gateways::cleanDevicesInfo( gateways::GatewayID::ZWave );

    if( !file.open( STORE_PATH, "wb" ) ) {
        ZWAVE_LOGE( "open store for reset fail!" );
        return -1;
    }
    return 0;
}

void ZNet::nodeCreate( znet_node_id_t nodeId ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        device = Gateway::get().addDevice( nodeId );  // TODO add checking of an id of an added device
        if( !device ) {
            return;
        }
    }
}

void ZNet::interviewDone( const uint8_t nodeId ) {
    auto device = Gateway::get().addDevice( nodeId );
    if( !device ) {
        return;
    }

    if( !DevicesStorage::get().addDeviceFile( device.get() ) ) {
        ZWAVE_LOGE( "znet api: add node fail" );
    }

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::updateState(
            subscriber, common::resources::broadcasts::events::NodeAdded, nodeId, {} );
    } );

    uint8_t channelId = 0;
    uint8_t commandClassId = 0x80;

    znet::ZNode::ClassPtr poll_cc = device->getCommandClass( channelId, commandClassId );
    if( poll_cc ) {
        m_pollTimer = createTimer( [nodeId, channelId, commandClassId]() {
            auto device = Gateway::get().getDevice( nodeId );
            if( !device ) {
                ZWAVE_LOGE( "RequestValue. Getting of device has been failed. Node's ID is %d", nodeId );
                return;
            }

            znet::ZNode::ClassPtr cc = device->getCommandClass( channelId, commandClassId );
            if( !cc ) {
                return;
            }

            if( !gateways::zwave::ZNet::get().isBusy() ) {
                znet::RequestValue request;
                request.resultCallback = []( znet::RequestFailTypeOpt failOpt ) {};
                cc->requestValue( request );
            }
        } );
        if( m_pollTimer ) {
            uint16_t interval = 2000;
            m_pollTimer->runPeriodically( std::chrono::milliseconds( 0 ), std::chrono::milliseconds( interval ) );
        }
    }
}

void ZNet::nodeRemove( const uint8_t nodeId ) {
    if( !isSubscribersEmpty() ) {
        if( auto device = Gateway::get().getDevice( nodeId ) ) {
            notify( [&]( std::string subscriber ) {
                return ::bus::lua_event::nodeRemoved( subscriber,
                                                      nodeId > 0,
                                                      device->getManufacturerId(),
                                                      device->getProductType(),
                                                      device->getProductId(),
                                                      nodeId );
            } );
        }
        else {
            ZWAVE_LOGW( "nodeRemove: didn't found device Id=%i!", nodeId );
            if( nodeId > 0 ) {
                notify( [&]( std::string subscriber ) {
                    return ::bus::lua_event::nodeRemoved( subscriber, true, 0, 0, 0, nodeId );
                } );
            }
        }
    }

    Gateway::get().removeDevice( nodeId );
}

void ZNet::onCheckReachable( uint8_t nodeId ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( device && ( device->isReachable() == znet::ReachableState::Reachable ) ) {
        device->resetDataUpdateTimestamp();
        Gateway::get().checkReachableDevicesState();
    }
}

void ZNet::onUpdateReachable( znet_node_id_t nodeId, bool reachable ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->onDataUpdated();
    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::reachableStateUpdated( subscriber, nodeId, reachable );
    } );
}

void ZNet::onDeviceIncludeProgressUpdate( uint8_t progress ) {
    notify( [&]( std::string subscriber ) { return ::bus::lua_event::includeDeviceProgress( subscriber, progress ); } );
}

void ZNet::onLearnProgressUpdate( znet_node_id_t devicesAmount, znet_node_id_t currentDeviceIndex ) {
    ZWAVE_LOGD( "devicesAmount = %d, currentDeviceIndex = %d", devicesAmount, currentDeviceIndex );
    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::learnProgress( subscriber, devicesAmount, currentDeviceIndex );
    } );
}

void ZNet::onReconstructNetworkUpdate( znet_node_id_t devicesAmount, znet_node_id_t currentDeviceIndex ) {
    ZWAVE_LOGD( "devicesAmount = %d, currentDeviceIndex = %d", devicesAmount, currentDeviceIndex );
    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::reconstructProgress( subscriber, devicesAmount, currentDeviceIndex );
    } );
}

void ZNet::onReconstructNetworkError( const znet_node_id_t deviceId ) {
    ZWAVE_LOGD( "deviceId = %d", deviceId );
    notify(
        [&]( std::string subscriber ) { return ::bus::lua_event::reconstructProgressError( subscriber, deviceId ); } );
}

void ZNet::onUpdateValue( const znet_node_id_t nodeId,
                          const uint8_t ccId,
                          const uint8_t channelId,
                          const uint8_t currentValue,
                          const znet::Uint8Opt targetValue,
                          const znet::SecondsOpt targetDuration ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        ZWAVE_LOGE( "The device has not been found" );
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::valueUpdated(
            subscriber, nodeId, channelId, ccId, currentValue, targetValue, targetDuration );
    } );
}

void ZNet::onUpdateThermostatModeValue( const uint8_t nodeId,
                                        const uint8_t channelId,
                                        const uint8_t mode,
                                        ::znet::ManufacturerSpecificDataOpt manufacturerSpecificData ) {
    getDeviceAndSetReachable( nodeId );

    std::optional<std::string> dataStr;
    if( manufacturerSpecificData ) {
        dataStr = ::znet::utils::bytesToHex( *manufacturerSpecificData );
    }

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::thermostatModeValueUpdated(
            std::move( subscriber ), nodeId, channelId, mode, dataStr );
    } );
}

void ZNet::onUpdateValueThermostatSetpoint( const uint8_t nodeId,
                                            const uint8_t channelId,
                                            const uint8_t type,
                                            const uint8_t scale,
                                            const float value,
                                            const bool isSetpointRequested ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }
    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::thermostatSetpointValueUpdated(
            subscriber,
            nodeId,
            channelId,
            std::make_unique<common::IntValue>( type ),   // TODO  remake to string using like meters
            std::make_unique<common::IntValue>( scale ),  // TODO  remake to string using like meters
            value,
            isSetpointRequested );
    } );
}

void ZNet::onUpdateTokenValue( const znet_node_id_t nodeId,
                               const uint8_t commandClassId,
                               const uint8_t channelId,
                               const std::string& value ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }
    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::tokenValueUpdated(
            subscriber, nodeId, channelId, commandClassId, ::common::IValue::create( value ) );
    } );
}

void ZNet::onUpdateValueAlarmSensor( const uint8_t nodeId,
                                     const uint8_t channelId,
                                     const znet::AlarmSensorType sensorType,
                                     const uint8_t value,
                                     const uint16_t seconds ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }
    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::alarmSensorValueUpdated(
            subscriber, nodeId, channelId, ::znet::getAlarmSensorName( sensorType ), seconds, value );
    } );
}

void ZNet::onUpdateValueBinarySensor( const uint8_t nodeId,
                                      const uint8_t ccId,
                                      const uint8_t channelId,
                                      const uint8_t sensorType,
                                      const uint8_t value ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }
    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::binarySensorValueUpdated( subscriber, nodeId, channelId, ccId, sensorType, value );
    } );
}

void ZNet::onUpdateValueMultilevelSensor( const uint8_t nodeId,
                                          const uint8_t ccId,
                                          const uint8_t channelId,
                                          const znet::MultilevelSensor::Type type,
                                          const uint8_t scale,
                                          const float value ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }
    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::multilevelSensorValueUpdated(
            subscriber,
            nodeId,
            channelId,
            ccId,
            ::common::IValue::create(::znet::multilevel_sensor_converter::typeToString( type ) ),
            ::common::IValue::create(::znet::multilevel_sensor_converter::scaleToString( type, scale ) ),
            value );
    } );
}

void ZNet::onUpdateValueDoorLockOperation( const uint8_t nodeId,
                                           const uint8_t channelId,
                                           const znet::DoorLock::Mode mode,
                                           const uint8_t inside_handle_mode,
                                           const uint8_t outside_handle_mode,
                                           const znet::DoorCondition condition,
                                           const znet::SecondsOpt lockTimeout,
                                           const znet::DoorLock::ModeOpt targetMode,
                                           const znet::SecondsOpt targetModeTimeout ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }
    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::doorLockStateUpdated(
            subscriber,
            nodeId,
            channelId,
            ::znet::door_lock_converter::modeToString( mode ),
            ::znet::door_lock_converter::latchStateToString( condition.latchState ),
            ::znet::door_lock_converter::boltStateToString( condition.boltState ),
            ::znet::door_lock_converter::doorStateToString( condition.doorState ),
            parseDoorLockHandleIds( inside_handle_mode ),
            parseDoorLockHandleIds( outside_handle_mode ),
            lockTimeout ? std::optional<uint16_t>{ lockTimeout->count() } : std::nullopt,
            targetMode ? std::optional<std::string>{ znet::door_lock_converter::modeToString( *targetMode ) } :
                         std::nullopt,
            targetModeTimeout ? std::optional<std::string>{ std::to_string( targetModeTimeout->count() ) } :
                                std::nullopt );
    } );
}

void ZNet::onUpdateValueDoorLockConfiguration( const uint8_t nodeId,
                                               const uint8_t channelId,
                                               const znet_door_lock_configuration_t& config,
                                               const znet_door_lock_extended_configuration_t* extConfig ) {
    std::optional<uint16_t> lockTimeout;
    if( config.are_timed_operations_enabled ) {
        lockTimeout = config.lock_timeout;
    }
    std::optional<::configuration::ExtDoorLockConfiguration> optionalConf;
    if( extConfig ) {
        optionalConf = { .m_autoRelockTimeout = extConfig->auto_relock_timeout,
                         .m_holdAndReleaseTimeout = extConfig->hold_and_release_timeout,
                         .m_twistAssistEnabled = extConfig->twist_assist_enabled,
                         .m_blockToBlockEnabled = extConfig->block_to_block_enabled };
    }
    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::doorLockConfigurationUpdated(
            subscriber,
            nodeId,
            channelId,
            parseDoorLockHandleIds( config.enabled_inside_handle_ids ),
            parseDoorLockHandleIds( config.enabled_outside_handle_ids ),
            lockTimeout,
            std::move( optionalConf ) );
    } );
}

void ZNet::onUserCodeUpdated( znet_node_id_t nodeId,
                              znet_node_channel_id_t channelId,
                              ::znet::UserCode::UserCodeRanges&& userCodeRanges,
                              znet_supported_users_t usersCount,
                              ::znet::UserCode::UserPollStatus userPollingStatus ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::userCodeUpdated(
            subscriber,
            nodeId,
            channelId,
            usersCount,
            ::znet::user_code_converter::userPollStatusToString( userPollingStatus ),
            userCodeRanges );
    } );
}

void ZNet::onUpdateFloatValue( const uint8_t nodeId,
                               const uint8_t ccId,
                               const uint8_t channelId,
                               const znet::meter::MeterType type,
                               const znet::meter::Scale scale,
                               const int32_t value,
                               const uint8_t precision ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::floatValueUpdated(
            subscriber,
            nodeId,
            channelId,
            ccId,
            ::common::IValue::create(::znet::meter::typeToString( type ) ),
            ::common::IValue::create(::znet::meter::scaleToString( type, scale ) ),
            precision,
            value );
    } );
}

void ZNet::onUpdateSceneValue( const uint8_t nodeId,
                               const uint8_t ccId,
                               const uint8_t channelId,
                               const uint8_t sequence_number,
                               const uint8_t sceneNumber,
                               const std::string& key_atribute ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    ZWAVE_LOGD( "ZNet::onUpdateSceneValue" );

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::buttonStateUpdated( subscriber, nodeId, channelId, ccId, sceneNumber, key_atribute );
    } );
}

void ZNet::onSceneActivated( const uint8_t nodeId,
                             const uint8_t ccId,
                             const uint8_t channelId,
                             const uint8_t sceneNumber ) {
    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::sceneActivated( subscriber, nodeId, channelId, ccId, sceneNumber );
    } );
}

void ZNet::onUpdateRgbValue( const uint8_t nodeId,
                             const uint8_t ccId,
                             const uint8_t channelId,
                             ::znet::ColorComponentInfos&& components ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    const std::map<::znet::ColorComponentID, ::common::RgbValue::Color> znetColorIdToColor = {
        { ::znet::ColorComponentID::Wwhite, ::common::RgbValue::Color::Wwhite },
        { ::znet::ColorComponentID::Cwhite, ::common::RgbValue::Color::Cwhite },
        { ::znet::ColorComponentID::Red, ::common::RgbValue::Color::Red },
        { ::znet::ColorComponentID::Green, ::common::RgbValue::Color::Green },
        { ::znet::ColorComponentID::Blue, ::common::RgbValue::Color::Blue },
        { ::znet::ColorComponentID::Amber, ::common::RgbValue::Color::Amber },
        { ::znet::ColorComponentID::Cyan, ::common::RgbValue::Color::Cyan },
        { ::znet::ColorComponentID::Purple, ::common::RgbValue::Color::Purple },
        { ::znet::ColorComponentID::Indexed, ::common::RgbValue::Color::Indexed }
    };
    std::vector<common::RgbValue::ColorComponent> colors;
    std::vector<common::RgbValue::ColorComponent> targetColors;
    std::vector<common::RgbValue::ColorComponent> targetDurations;
    for( const ::znet::ColorComponentInfo& component : components ) {
        auto color = znetColorIdToColor.at( component.id );
        colors.emplace_back( color, component.value );
        if( component.targetValue ) {
            targetColors.emplace_back( color, *component.targetValue );
        }
        if( component.targetDuration ) {
            targetDurations.emplace_back( color, component.targetDuration->count() );
        }
    }

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::rgbValueUpdated( subscriber,
                                                  nodeId,
                                                  channelId,
                                                  std::make_unique<common::RgbValue>( std::move( colors ) ),
                                                  std::make_unique<common::RgbValue>( std::move( targetColors ) ),
                                                  std::make_unique<common::RgbValue>( std::move( targetDurations ) ) );
    } );
}

void ZNet::onUpdateValueThermostatOperatingState( const uint8_t nodeId,
                                                  const uint8_t channelId,
                                                  const uint8_t command,
                                                  znet::ThermostatOperatingState state ) {
    // TODO::
    ZWAVE_LOGW( ">>>> Need to implement onUpdateValueThermostatOperatingState <<<<" );
    return;
}

void ZNet::onUpdateThermostatSetbackValue( const uint8_t nodeId,
                                           const uint8_t channelId,
                                           const znet::thermostat_setback::OverrideType overrideType,
                                           const znet::thermostat_setback::Mode mode,
                                           const znet::thermostat_setback::SetbackValueOpt valueOpt ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }
    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::thermostatSetbackValueUpdated(
            subscriber, nodeId, channelId, overrideType, mode, valueOpt );
    } );
}

void ZNet::onUpdateSoundSwitchConfiguration( znet_node_id_t nodeId,
                                             znet_node_channel_id_t channelId,
                                             const znet_cmd_sound_switch_configuration_t& value ) {
    ZWAVE_LOGD( "nodeId=%d, channelId=%d, volume=%d, defaultToneId=%d",
                nodeId,
                channelId,
                value.volume,
                value.default_tone_id );
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::soundSwitchConfiguration(
            subscriber, nodeId, channelId, value.volume, value.default_tone_id );
    } );
}

void ZNet::onUpdateSoundSwitchTonePlay( znet_node_id_t nodeId,
                                        znet_node_channel_id_t channelId,
                                        const znet_cmd_sound_switch_tone_play_t& value ) {
    ZWAVE_LOGD( "nodeId=%d, channelId=%d, toneId=%d, volume=%d", nodeId, channelId, value.tone_id, value.volume );
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::soundSwitchTonePlay( subscriber, nodeId, channelId, value.tone_id, value.volume );
    } );
}

void ZNet::onProvisioningListEntryAdded( const ::znet::ProvisioningListEntry& entry ) {
    send( "nma", std::make_unique<::gateways::commands::ProvisioningListEntryAdded>( entry ) );
}

void ZNet::onProvisioningListEntryUpdated( const ::znet::ProvisioningListEntry& oldEntry,
                                           const ::znet::ProvisioningListEntry& newEntry ) {
    ::znet::ProvisioningListEntryFields diff;
    if( newEntry.m_fields.m_node != oldEntry.m_fields.m_node ) {
        diff.m_node = newEntry.m_fields.m_node;
    }
    if( newEntry.m_fields.m_securityModes != oldEntry.m_fields.m_securityModes ) {
        diff.m_securityModes = newEntry.m_fields.m_securityModes;
    }
    if( newEntry.m_fields.m_nodeSecurityModes != oldEntry.m_fields.m_nodeSecurityModes ) {
        diff.m_nodeSecurityModes = newEntry.m_fields.m_nodeSecurityModes;
    }
    if( newEntry.m_fields.m_supportedModes != oldEntry.m_fields.m_supportedModes ) {
        diff.m_supportedModes = newEntry.m_fields.m_supportedModes;
    }
    if( newEntry.m_fields.m_room != oldEntry.m_fields.m_room ) {
        diff.m_room = newEntry.m_fields.m_room;
    }
    if( newEntry.m_fields.m_name != oldEntry.m_fields.m_name ) {
        diff.m_name = newEntry.m_fields.m_name;
    }
    if( newEntry.m_fields.m_uuid != oldEntry.m_fields.m_uuid ) {
        diff.m_uuid = newEntry.m_fields.m_uuid;
    }
    if( newEntry.m_fields.m_maxRequestInterval != oldEntry.m_fields.m_maxRequestInterval ) {
        diff.m_maxRequestInterval = newEntry.m_fields.m_maxRequestInterval;
    }
    if( newEntry.m_fields.m_addingMode != oldEntry.m_fields.m_addingMode ) {
        diff.m_addingMode = newEntry.m_fields.m_addingMode;
    }
    if( newEntry.m_fields.m_status != oldEntry.m_fields.m_status ) {
        diff.m_status = newEntry.m_fields.m_status;
    }
    if( newEntry.m_fields.m_product != oldEntry.m_fields.m_product ) {
        diff.m_product = newEntry.m_fields.m_product;
    }
    if( newEntry.m_fields.m_type != oldEntry.m_fields.m_type ) {
        diff.m_type = newEntry.m_fields.m_type;
    }
    send( "nma",
          std::make_unique<::gateways::commands::ProvisioningListEntryUpdated>( newEntry.m_provisioningId, diff ) );
}

void ZNet::onProvisioningListEntryRemoved( uint32_t provisioningId ) {
    send( "nma", std::make_unique<::gateways::commands::ProvisioningListEntryRemoved>( provisioningId ) );
}

void ZNet::znetS2SelectKeys( znet_S2_keys_t keyList, void* arg ) {
    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::s2SelectAuthenticationModes(
            subscriber, ::znet::security_modes_converter::bitsetToList( keyList ) );
    } );
}

void ZNet::znetS2RequestPubKey( znet_S2_keys_t /*keyList*/, const znet_S2_pub_key_t* key, void* arg ) {
    ZWAVE_LOGD( "ZNet::znetS2RequestPubKey" );
    if( key ) {
        uint8_t key_count = 0;
        const char* eventName;
        if( key->size == 14 ) {
            key_count = 7;
            eventName = ::common::resources::broadcasts::events::S2RequestDeviceSpecificKey;
        }
        else if( key->size == 16 ) {  // CSA
            key_count = 2;
            eventName = ::common::resources::broadcasts::events::S2ShowDeviceSideKey;
        }
        std::vector<uint16_t> keyParsed;
        for( uint8_t i = 0; i < key_count; ++i ) {
            keyParsed.push_back( ( (uint16_t)key->key[i * 2] << 8 ) | key->key[i * 2 + 1] );
        }

        notify( [&]( std::string subscriber ) {
            return ::bus::lua_event::s2PublicKeyReceived( subscriber, std::move( keyParsed ), eventName );
        } );
    }
}

void ZNet::znetS2ShowPubKey( znet_S2_keys_t /*keyList*/, const znet_S2_pub_key_t* key, void* arg ) {
    ZWAVE_LOGD( "znetS2ShowPubKey  " );
    if( key ) {
        std::vector<uint16_t> keyParsed;
        for( uint8_t i = 0; i < key->size / 2; ++i ) {
            keyParsed.push_back( ( (uint16_t)key->key[i * 2] << 8 ) | key->key[i * 2 + 1] );
        }

        notify( [&]( std::string subscriber ) {
            return ::bus::lua_event::s2PublicKeyReceived(
                subscriber, std::move( keyParsed ), ::common::resources::broadcasts::events::LearnShowKey );
        } );
    }
}

void ZNet::zwaveModuleResetPinRelease() {
    ::hard::ResetZW::get().resetPinRelease();
}

void ZNet::zwaveModuleResetPinAssert() {
    ::hard::ResetZW::get().resetPinAssert();
}

bool ZNet::getKnownS0Key( uint8_t* ) {
    // Vera migration
    return false;
}

void ZNet::onSisUpdated( bool enabled ) {
    notify( [&]( std::string subscriber ) { return ::bus::lua_event::sisUpdate( subscriber ); } );
}

void ZNet::onWakeupIntervalUpdated( znet_node_id_t nodeId, std::chrono::seconds ) {
    if( auto device = ::gateways::zwave::Gateway::get().getDevice( nodeId ) ) {
        device->setReachable(::znet::ReachableState::Reachable );
    }
}

void ZNet::fillManufacturerSpecific( znet_manufacturer_specific_report_t& data ) {
    data.product_id = getProductId();
    data.product_type = getProductType();
    data.manufacturer_id = getManufacturerId();
}

void ZNet::fillVersion( znet_version_report_t& data ) {
    data.hard_ver = getHardwareVersion();
    const auto chipVersion = getZwaveChipFirmwareVersion();
    data.proto.ver = chipVersion.first;
    data.proto.sub_ver = chipVersion.second;

    const auto mainVersion = getHubFirmwareVersion();

    data.firms_count = 2;
    data.firms[0].ver = chipVersion.first;
    data.firms[0].sub_ver = chipVersion.second;
    data.firms[1].ver = mainVersion.first;
    data.firms[1].sub_ver = mainVersion.second;
}

void ZNet::fillZWavePlusInfo( znet_zwaveplus_info_report_t& data ) {
    data.installer_icon = getInstallerIconType();
    data.user_icon = getUserIconType();
}

void ZNet::fillFirmwareLocale( std::string& locale ) {
    locale = getZwaveChipLocale();
}

void ZNet::fillFirmwareUpdateInfo( znet_firmware_md_report_v1_t& dataV1,
                                   znet_firmware_md_report_v3_t& dataV3,
                                   znet_firmware_md_report_v5_t& dataV5 ) {
    dataV1.firmware_id = getZwaveChipFirmwareId();
    dataV1.manufacturer_id = getManufacturerId();

    dataV3.additional_firmware_count = 1;
    dataV3.additional_firmware[0] = getHubFirmwareId();

    dataV5.hardware_revision = getHardwareVersion();
}

void ZNet::onConfigurationReport( const uint8_t nodeId,
                                  const uint8_t channelId,
                                  const uint16_t configParamNum,
                                  const uint8_t configParamSize,
                                  const uint32_t znetConfigParamValue ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    znet::ZNode::ClassPtr ccBase = device->getCommandClass( ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION );
    if( !ccBase ) {
        ZWAVE_LOGE( "Cannot get ZCc0x70Configuration. nodeId=%d0x", nodeId );
        return;
    }
    auto cc = static_cast<znet::ZCc0x70Configuration*>( ccBase.get() );
    int64_t uiConfigParamValue = znetConfigParamValue;
    if( auto parameter = cc->findV3Parameter( configParamNum ) ) {
        uiConfigParamValue = ::znet::signed_configuration_parameter_converter::toUiValue(
            znetConfigParamValue, configParamSize, parameter->m_format );
    }
    send(::bus::lua_event::BUS_NAME,
         ::bus::lua_event::configurationValueUpdated( nodeId,
                                                      channelId,
                                                      ZNET_COMMAND_CLASS_CONFIGURATION,
                                                      configParamNum,
                                                      configParamSize,
                                                      uiConfigParamValue ) );
}

void ZNet::onConfigurationRequestStatus( const uint8_t nodeId,
                                         const uint8_t configParamNum,
                                         znet::RequestStatus::Enum status ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    send(::bus::lua_event::BUS_NAME,
         ::bus::lua_event::configurationRequestStatusUpdated(
             nodeId, configParamNum, ::znet::RequestStatus::toString( status ) ) );
}

void ZNet::onNotification( const uint8_t nodeId,
                           const uint8_t channelId,
                           const uint8_t v1AlarmType,
                           const uint8_t v1AlarmLevel,
                           znet::NotificationType notification,
                           znet::NotificationEvent event,
                           const uint8_t* data,
                           const uint8_t dataSize ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    const ::znet::NotificationEventData* eventData = ::znet::getNotificationEventData( notification, event );

    size_t encoded_size{ 0 };
    ::mbedtls_base64_encode( nullptr, 0, &encoded_size, (const unsigned char*)data, dataSize );
    std::vector<char> decoded_code( encoded_size + 1, 0 );
    auto raw64 = reinterpret_cast<unsigned char*>( decoded_code.data() );
    mbedtls_base64_encode( raw64, encoded_size, &encoded_size, (const unsigned char*)data, dataSize );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::notificationReceived(
            subscriber,
            nodeId,
            channelId,
            ::znet::getNotificationName( notification ),
            v1AlarmType,
            v1AlarmLevel,
            eventData->m_name,
            reinterpret_cast<const char*>( raw64 ),
            dataSize > 0 && eventData->m_valueHandler ?
                eventData->m_valueHandler->getNotificationValue( data, dataSize ) :
                ::znet::NotificationValueOpt{} );
    } );
}

void ZNet::onUnknownNotification( const uint8_t nodeId,
                                  const uint8_t channelId,
                                  const uint8_t v1AlarmType,
                                  const uint8_t v1AlarmLevel,
                                  znet::NotificationType notification,
                                  znet::NotificationEvent event,
                                  const uint8_t* data,
                                  const uint8_t dataSize ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Unknown );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::unknownNotificationReceived( subscriber,
                                                              nodeId,
                                                              channelId,
                                                              ::znet::getNotificationName( notification ),
                                                              v1AlarmType,
                                                              v1AlarmLevel,
                                                              event );
    } );
}

void ZNet::onAssociationSupportedGroupingResult( const uint8_t nodeId, const uint8_t count ) {
    auto device = Gateway::get().getDevice( nodeId );

    if( !device ) {
        return;
    }
    using namespace common::resources;

    ZWAVE_LOGD( "Read parameter: nodeId : %d, groups : %d", nodeId, count );

    //    nma::BroadcastsCreator::createFoGatewayAssocGroupMethod( m_currentCallMethodName,
    //                                                             common::resources::broadcasts::events::Done,
    //                                                             gateways::GatewayID::ZWave,
    //                                                             nodeId,
    //                                                             count );
}

void ZNet::onAssociationResult( const uint8_t nodeId,
                                const uint8_t channelId,
                                const uint8_t groupId,
                                const std::vector<uint8_t>& nodes ) {
    // TODO association_updated  by DOC
    ZWAVE_LOGE( "Need implement !!!!!!!!!!!!!!!!!!!!!!" );
}

void ZNet::onMultichannelAssociationResult( const uint8_t nodeId,
                                            const uint8_t channelId,
                                            const uint8_t groupId,
                                            const std::vector<std::pair<uint8_t, uint8_t>>& nodes ) {
    // TODO association_updated  by DOC
    ZWAVE_LOGE( "Need implement !!!!!!!!!!!!!!!!!!!!!!" );
}

void ZNet::onMasterCodeUpdated( const uint8_t nodeId, const uint8_t channelId, const std::string& masterCode ) {
    if( auto device = getDeviceAndSetReachable( nodeId ) ) {
        notify( [&]( std::string subscriber ) {
            return ::bus::lua_event::masterCodeUpdated(
                std::move( subscriber ), nodeId, channelId, std::move( masterCode ) );
        } );
    }
}

void ZNet::onUserCodesKeypadModeUpdated( const uint8_t nodeId,
                                         const uint8_t channelId,
                                         znet::UserCode::KeypadMode keypadMode ) {
    if( auto device = getDeviceAndSetReachable( nodeId ) ) {
        notify( [&]( std::string subscriber ) {
            return ::bus::lua_event::userCodesKeypadModeUpdated(
                std::move( subscriber ),
                nodeId,
                channelId,
                ::znet::user_code_converter::keypadModeToString( keypadMode ) );
        } );
    }
}

void ZNet::onUpdateScheduleEntryLockDailyRepeating( znet_node_id_t nodeId,
                                                    znet_node_channel_id_t channelId,
                                                    const znet_daily_repeating_schedule_t& value ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        ZWAVE_LOGE( "Gateway::get().getDevice( nodeId=%d ) returned null", nodeId );
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::dailyRepeatingEntryLockSchedule(
            subscriber,
            nodeId,
            channelId,
            value.schedule_id.user_id,
            value.schedule_id.slot_id,
            ::is_daily_repeating_valid( &value ) && value.week_day_bitmask ?
                std::optional<znet_daily_repeating_schedule_t>{ value } :
                std::nullopt );
    } );
}

void ZNet::onUpdateScheduleEntryLockWeekDay( znet_node_id_t nodeId,
                                             znet_node_channel_id_t channelId,
                                             const znet_week_day_schedule_t& value ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        ZWAVE_LOGE( "Gateway::get().getDevice( nodeId=%d ) returned null", nodeId );
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::weekDayEntryLockSchedule(
            subscriber,
            nodeId,
            channelId,
            value.schedule_id.user_id,
            value.schedule_id.slot_id,
            ::is_week_day_schedule_valid( &value ) ? std::optional<znet_week_day_schedule_t>{ value } : std::nullopt );
    } );
}

void ZNet::onUpdateScheduleEntryLockYearDay( znet_node_id_t nodeId,
                                             znet_node_channel_id_t channelId,
                                             const znet_year_day_schedule_t& value ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        ZWAVE_LOGE( "Gateway::get().getDevice( nodeId=%d ) returned null", nodeId );
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::yearDayEntryLockSchedule(
            subscriber,
            nodeId,
            channelId,
            value.schedule_id.user_id,
            value.schedule_id.slot_id,
            ::is_year_day_schedule_valid( &value ) ? std::optional<znet_year_day_schedule_t>{ value } : std::nullopt );
    } );
}

void ZNet::onHail( const uint8_t nodeId ) {
    notify( [&]( std::string subscriber ) { return ::bus::lua_event::hail( subscriber, nodeId, 0 ); } );
}

void ZNet::onNif( const uint8_t nodeId ) {
    notify( [&]( std::string subscriber ) { return ::bus::lua_event::nif( subscriber, nodeId, 0 ); } );
}

bool ZNet::isAllDevicesRemoved() {
    return m_allDevicesRemoved;
}

void ZNet::onDeviceRediscoverProgressUpdate( znet_node_id_t nodeId,
                                             bool isOk,
                                             ::znet::ErrorOpt errorMessage,
                                             uint8_t progress ) {
    ZWAVE_LOGD( "progress %d", progress );

    if( isOk && progress == 100 ) {
        auto device = Gateway::get().addDevice( nodeId );
        if( !device ) {
            return;
        }

        if( !DevicesStorage::get().addDeviceFile( device.get() ) ) {
            ZWAVE_LOGE( "znet api: add node fail" );
        }
    }

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::rediscoverDeviceProgress(
            subscriber, nodeId, progress, ::znet::utils::progressToStringStatus( isOk, progress ), errorMessage );
    } );
}

class FirmwareUpdateSource : public znet::FirmwareUpdate::Source {
public:
    FirmwareUpdateSource( std::shared_ptr<std::ifstream> stream )
    : m_stream( std::move( stream ) ) {}

    size_t read( size_t size, uint8_t* dst ) override {
        return static_cast<std::size_t>(
            m_stream->read( reinterpret_cast<char*>( dst ), static_cast<std::streamsize>( size ) ).gcount() );
    }

    void seek( size_t pos ) override {
        m_stream->clear();
        m_stream->seekg( static_cast<std::streamsize>( pos ), std::ios::beg );
    }

private:
    std::shared_ptr<std::ifstream> m_stream;
};

// DeviceFirmwareUpdateManager::Delegate
bool ZNet::startFirmwareUpdate( const common::DeviceFirmwareUpdateManager::Device& device,
                                const common::DeviceFirmwareUpdateManager::Target& target,
                                const common::DeviceFirmwareUpdateManager::Path& destination,
                                std::function<void( bool )> resultCallback ) {
    ZWAVE_LOGI( "device: %d", device );
    ZWAVE_LOGI( "target: %d", target );
    ZWAVE_LOGI( "destination: %s", destination.c_str() );

    core::Thread thread(
        "UpdateNodeTargetFirmwareTask", 10 * 1024, updateNodeTargetFirmwareTask, device, target, destination );
    if( thread ) {
        thread.detach();
    }

    return true;
}

bool ZNet::startChipBackup( bool backupCreation, std::function<void( bool )> resultCallback ) {
    core::Thread thread( "ChipBackupTask", 6 * 1024, chipBackupTask, backupCreation, resultCallback );
    if( thread ) {
        thread.detach();
    }

    return true;
}

void ZNet::chipBackupTask( bool backupCreation, std::function<void( bool )> resultCallback ) {
    ZWAVE_LOGD( "chipBackupTask" );
    const auto chipFileName = ::common::resources::tokens::ChipBackupFile;
    if( backupCreation ) {
        auto stream = std::make_shared<std::ofstream>( chipFileName, std::ofstream::binary );
        if( !stream->is_open() ) {
            ZWAVE_LOGE( "Create backup file failed" );
            resultCallback( false );
        }
        else {
            gateways::zwave::ZNet::get().createChipBackup( std::make_unique<cloud_storage::FileDestination>( stream ) );
        }
    }
    else {
        auto stream = std::make_shared<std::ifstream>( chipFileName, std::ifstream::binary );
        if( !stream->is_open() ) {
            ZWAVE_LOGE( "Open backup file failed" );
            resultCallback( false );
        }
        else {
            gateways::zwave::ZNet::get().restoreChipBackup( std::make_unique<cloud_storage::FileSource>( stream ) );
        }
    }

    resultCallback( true );
}

void ZNet::updateNodeTargetFirmwareTask( const common::DeviceFirmwareUpdateManager::Device& device,
                                         const common::DeviceFirmwareUpdateManager::Target& target,
                                         const common::DeviceFirmwareUpdateManager::Path& destination ) {
    ZWAVE_LOGI( "device: %d", device );
    ZWAVE_LOGI( "target: %d", target );
    ZWAVE_LOGI( "destination: %s", destination.c_str() );

    long begin, end;
    std::ifstream myfile( destination.c_str() );
    begin = myfile.tellg();
    myfile.seekg( 0, std::ios::end );
    end = myfile.tellg();
    myfile.close();
    int size = end - begin;
    ZWAVE_LOGI( "destination file size: %d: ", size );

    std::shared_ptr<std::ifstream> stream = std::make_shared<std::ifstream>( destination, std::ifstream::binary );
    if( !stream->is_open() ) {
        return;
    }

    gateways::zwave::ZNet::get().updateNodeTargetFirmware(
        static_cast<znet_node_id_t>( device ),
        static_cast<uint8_t>( target ),
        std::make_unique<FirmwareUpdateSource>( std::move( stream ) ) );
}

void ZNet::onFirmwareDownloadStatus( const common::DeviceFirmwareUpdateManager::ExtendedLink& link,
                                     bool isOk,
                                     uint8_t progress ) {
    ZWAVE_LOGI( "link.image: %s", link.image.c_str() );
    ZWAVE_LOGI( "link.checksum: %s", link.checksum.c_str() );
    ZWAVE_LOGI( "progress: %d", progress );

    auto truncateLink = []( const std::string& name ) {
        const auto pos = name.rfind( '/' );
        return name.substr( pos == std::string::npos ? 0 : ( pos + 1 ) );
    };

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::firmwareDownloadStatus( subscriber,
                                                         truncateLink( link.image ),
                                                         truncateLink( link.checksum ),
                                                         znet::utils::progressToStringStatus( isOk, progress ),
                                                         progress );
    } );
}

void ZNet::addFirmwareUpdateTask( std::function<void()> task ) {
    ZWAVE_LOGI( "ZNet::addFirmwareUpdateTask" );

    core::Thread thread( "UpdateNodeTargetFirmwareTask", 6 * 1024, task );
    if( thread ) {
        thread.detach();
    }
}

void ZNet::onFirmwareUpdateStarted( const common::DeviceFirmwareUpdateManager::Device& device ) {
    ZWAVE_LOGI( "ZNet::onFirmwareUpdateStarted: %d", device );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::firmwareUpdateStatus( subscriber, static_cast<uint8_t>( device ), {}, "started" );
    } );
}

void ZNet::onFirmwareUpdateFinished( const common::DeviceFirmwareUpdateManager::Device& device,
                                     const common::DeviceFirmwareUpdateManager::TargetsUpdateResults& targetsResults ) {
    ZWAVE_LOGI( "!!ZNet::onFirmwareUpdateFinished: %d", device );

    const auto allFailed = std::none_of(
        targetsResults.begin(),
        targetsResults.end(),
        []( const common::DeviceFirmwareUpdateManager::TargetsUpdateResults::value_type& targetToResult ) {
            return targetToResult.second == common::DeviceFirmwareUpdateManager::TargetUpdateResult::UpdateDone;
        } );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::firmwareUpdateStatus(
            subscriber, static_cast<uint8_t>( device ), {}, allFailed ? "failed" : "finished" );
    } );
}

void ZNet::onNodeTargetFirmwareUpdateProgress( const znet_node_id_t nodeId,
                                               const uint8_t target,
                                               ::znet::FirmwareUpdate::UpdatingResult status,
                                               uint8_t progress ) {
    auto statusStr =
        znet::utils::progressToStringStatus( status != znet::FirmwareUpdate::UpdatingResult::Error, progress );
    ZWAVE_LOGI( "Device firmware update status for node %d, target %d, status \"%s\" , progress %d",
                static_cast<uint8_t>( nodeId ),
                target,
                statusStr,
                progress );

    if( status != znet::FirmwareUpdate::UpdatingResult::InProgress ) {
        ZWAVE_LOGE( "status != znet::FirmwareUpdate::UpdatingResult::InProgress" );
        m_deviceFirmwareUpdateManager->onFirmwareUpdateFinished(
            nodeId, target, status != znet::FirmwareUpdate::UpdatingResult::Error );
    }

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::firmwareUpdateStatus(
            subscriber, static_cast<uint8_t>( nodeId ), target, statusStr, progress );
    } );
}

void ZNet::onIndicatorIdentify() {
    ::hard::LedControl::onIndicatorIdentify();
}

void ZNet::onUpdateBarrierSignalValue( znet_node_id_t nodeId,
                                       znet_node_channel_id_t channelId,
                                       uint8_t signalId,
                                       uint8_t value ) {
    ZWAVE_LOGD( "nodeId=%d, channelId=%d, signalId=%d, value=%d", nodeId, channelId, signalId, value );
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        return;
    }

    device->setReachable( znet::ReachableState::Reachable );

    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::updateBarrierSignal( subscriber,
                                                      static_cast<uint8_t>( nodeId ),
                                                      static_cast<uint8_t>( channelId ),
                                                      ZNET_COMMAND_CLASS_BARRIER_OPERATOR,
                                                      znet::barrier_operator_signal_converter::signalToString(
                                                          static_cast<znet::BarrierOperatorSignal>( signalId ) ),
                                                      value );
    } );
}

void ZNet::onNodeCheckStatusUpdated( uint8_t nodeId, ::znet::ModeStatus status ) {
    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::updateNodeStatus(
            subscriber, nodeId, znet::mode_status_converter::toString( status ) );
    } );
}

void ZNet::onAntiTheftStateUpdated( znet_node_id_t nodeId,
                                    znet_node_channel_id_t channelId,
                                    znet_command_class_t commandClassId,
                                    const ::znet::AntiTheftInfoOpt& antiTheftInfo ) {
    notify( [&]( std::string subscriber ) {
        return ::bus::lua_event::antiTheftStateUpdated( subscriber, nodeId, channelId, commandClassId, antiTheftInfo );
    } );
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::SendNif& command ) {  // TODO  add initiating of this command
    gateways::zwave::ZNet::get().sendNif();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::HardReset& command ) {
    gateways::zwave::ZNet::get().resetZwaveModule( true );
    constexpr unsigned int n_attempts = 3;
    constexpr int wait_ms = 1000;
    common::waitWhileFunctionIsTrueOrTimeout(
        []() { return gateways::zwave::ZNet::get().isResetting(); }, std::chrono::milliseconds( wait_ms ), n_attempts );
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StartInclude& command ) {
    m_deviceSettingsFilePath = command.getDeviceSettingsFilepath();
    gateways::zwave::ZNet::get().startInclude();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StopInclude& command ) {
    gateways::zwave::ZNet::get().stopInclude();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StopSecurityInclude& command ) {
    gateways::zwave::ZNet::get().stopSecurityInclude();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StartLearn& command ) {
    gateways::zwave::ZNet::get().startLearn();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StopLearn& command ) {
    gateways::zwave::ZNet::get().stopLearn();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StartExclude& command ) {
    uint8_t nodeId = std::atoi( command.m_nodeId.c_str() );
    //  TODO  extract id from a storage (after storage has been modified )

    // Check if received device settings are valid
    //    if( m_deviceId &&
    //        !gateways::utils::extractDeviceIdFromPublicId( m_deviceId.get<char>(), gatewayId, deviceId ) ) {
    //        return;
    //    }
    gateways::zwave::ZNet::get().startExclude( nodeId );
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StopExclude& command ) {
    gateways::zwave::ZNet::get().stopExclude();
}
void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StartRediscover& command ) {
    m_deviceSettingsFilePath = command.getDeviceSettingsFilepath();
    gateways::zwave::ZNet::get().interviewNode( command.getNodeId() );
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::GetBackgrounRSSI& command ) {
    gateways::zwave::ZNet::get().getBackgroundRSSI();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ReconstructNetwork& command ) {
    gateways::zwave::ZNet::get().reconstructNetwork();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::S2SelectKey& command ) {
    znet_S2_keys_t selected_keys = 0;

    for( std::string mode : command.m_authModes ) {
        size_t size = mode.size();

        if( strncmp( mode.c_str(), common::resources::broadcasts::s2::modes::S2, size ) == 0 ) {
            selected_keys |= ZNET_SECURITY2_S2_CLASS_2;
        }
        else if( strncmp( mode.c_str(), common::resources::broadcasts::s2::modes::S2Authenticated, size ) == 0 ) {
            selected_keys |= ZNET_SECURITY2_S2_CLASS_1;
        }
        else if( strncmp( mode.c_str(), common::resources::broadcasts::s2::modes::S2Unauthenticated, size ) == 0 ) {
            selected_keys |= ZNET_SECURITY2_S2_CLASS_0;
        }
        else if( strncmp( mode.c_str(), common::resources::broadcasts::s2::modes::S0, size ) == 0 ) {
            selected_keys |= ZNET_SECURITY2_S0_NET_KEY;
        }
        else if( strncmp( mode.c_str(), common::resources::broadcasts::s2::modes::CSA, size ) == 0 ) {
            selected_keys |= 0x40;  // ZNET_SECURITY2_S2_CSA
        }
    }
    gateways::zwave::ZNet::get().s2SelectKeys( selected_keys );
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::S2SetPublicKey& command ) {
    const size_t buffSize = sizeof( znet_S2_pub_key_t ) + 2 * sizeof( ( (znet_S2_pub_key_t*)nullptr )->key[0] );
    uint8_t buff[buffSize] = {};

    znet_S2_pub_key_t* pubKey = (znet_S2_pub_key_t*)&buff;
    pubKey->size = 2;

    // Workaround for compiler bug in release builds
    *( &( pubKey->key[0] ) ) = ( command.m_key >> 8 ) & 0xFF;
    pubKey->key[1] = command.m_key & 0xFF;

    gateways::zwave::ZNet::get().setPublicKey( pubKey );
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) {
    if( command.isSubscribe() ) {
        subscribe( from, command.handlerName() );
    }
    else {
        unsubscribe( bus::BusAddrInvalid, command.handlerName() );
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetValue& command ) {
    if( !command.m_value ) {
        ZWAVE_LOGE( "The item's value is empty " );
        return;
    }

    uint8_t& classId = command.m_commandClassId;
    uint8_t& channelId = command.m_channelId;

    auto device = Gateway::get().getDevice( command.m_nodeId );
    if( !device ) {
        ZWAVE_LOGE( "SetValue. Getting of device has been failed. Node's ID is %d", command.m_nodeId );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( channelId, classId );
    if( !cc ) {
        ZWAVE_LOGE(
            "SetValue. Getting of CC has been failed .  Node's ID is %d, CC's ID is 0x%x", command.m_nodeId, classId );
        return;
    }

    struct CmdVisitor : common::IValueVisitorConst {
        bus::command::SetValue& m_Command;
        znet::ZNode::ClassPtr& m_ComClass;
        uint8_t& m_ClassId;
        bool m_IsSuccessfull = false;

        CmdVisitor( bus::command::SetValue& command,
                    znet::ZNode::ClassPtr& comClass,
                    uint8_t& classId,
                    uint8_t& channelId )
        : m_Command( command )
        , m_ComClass( comClass )
        , m_ClassId( classId ) {}

        void visit( const common::FloatValue& value ) override {
            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValue( m_Command.m_nodeId,
                                                                                 m_Command.m_channelId,
                                                                                 m_Command.m_commandClassId,
                                                                                 static_cast<uint8_t>( value.get() ) );
            m_IsSuccessfull = true;
        };

        void visit( const common::ScalebleValue& value ) override{};

        void visit( const common::IntValue& value ) override {
            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValue( m_Command.m_nodeId,
                                                                                 m_Command.m_channelId,
                                                                                 m_Command.m_commandClassId,
                                                                                 static_cast<uint8_t>( value.get() ) );
            m_IsSuccessfull = true;
        };

        void visit( const common::Int64Value& value ) override {
            visit(::common::IntValue{ static_cast<int>( value.get() ) } );
            m_IsSuccessfull = true;
        };

        void visit( const common::BoolValue& value ) override {
            auto bVal = value.get() ? common::BoolValue::High : common::BoolValue::Low;
            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValue(
                m_Command.m_nodeId, m_Command.m_channelId, m_Command.m_commandClassId, bVal );
            m_IsSuccessfull = true;
        };

        void visit( const common::StringValue& value ) override {
            const auto& str = value.get();
            if( m_ClassId == ZNET_COMMAND_CLASS_SWITCH_MULTILEVEL ) {
                if( auto command = ::znet::stringToValueChangeCommand( str.c_str() ) ) {
                    auto ccTemp = static_cast<znet::ZCc0x26SwitchMultiLevel*>( m_ComClass.get() );
                    m_IsSuccessfull = ccTemp->setLevelChange( *command );
                }
            }
            else {
                if( m_ClassId == ZNET_COMMAND_CLASS_DOOR_LOCK ) {
                    auto command = znet::door_lock_converter::modeFromString( str.c_str() );
                    if( command ) {
                        auto ccTemp = static_cast<znet::ZCc0x62DoorLock*>( m_ComClass.get() );
                        m_IsSuccessfull = ccTemp->setOperation( static_cast<znet::DoorLock::Mode>( *command ) );
                    }
                }
                else {
                    if( m_ClassId == ZNET_COMMAND_CLASS_BARRIER_OPERATOR ) {
                        gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValue(
                            m_Command.m_nodeId,
                            m_Command.m_channelId,
                            m_Command.m_commandClassId,
                            static_cast<std::string>( value.get() ) );
                        m_IsSuccessfull = true;
                    }
                    else {
                        // INFO  this is a temporary solution for  JSON message { ... method":"setItemValue"
                        // .... "value":"true"} from nma bool value as a string
                        bool isTrue = str == "true";
                        auto bVal = isTrue ? common::BoolValue::High : common::BoolValue::Low;
                        gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValue(
                            m_Command.m_nodeId, m_Command.m_channelId, m_Command.m_commandClassId, bVal );
                        m_IsSuccessfull = true;
                    }
                }
            }
        };

        void visit( const common::RgbValue& value ) override {
            auto rgbVal = value.get();
            znet::ColorComponents znetColorComponents;
            znetColorComponents.reserve( rgbVal.size() );
            for( const common::RgbValue::ColorComponent& component : rgbVal ) {
                znet_cl_component_t znetColorComponent = {
                    static_cast<uint8_t>( rgbValueColorToZnetColor( component.color ) ), component.value
                };
                znetColorComponents.push_back( znetColorComponent );
            }

            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValue(
                m_Command.m_nodeId, m_Command.m_channelId, m_Command.m_commandClassId, znetColorComponents );
            m_IsSuccessfull = true;
        };

        void visit( const common::TokenValue& value ) override {
            ZWAVE_LOGE( "Not implemented Token" );
            m_IsSuccessfull = false;
        };

        void visit( const common::NotificationValue& value ) override {
            ZWAVE_LOGE( "Not implemented Notification" );
            m_IsSuccessfull = false;
        };
        void visit( const common::UserCodeValue& value ) override {
            ZWAVE_LOGE( "Not implemented UserCode" );
            m_IsSuccessfull = false;
        };
        void visit( const common::ArrayValue& value ) override {
            ZWAVE_LOGE( "Not implemented Array" );
            m_IsSuccessfull = false;
        };
        void visit( const common::TableValue& value ) override {
            if( m_ClassId == ZNET_COMMAND_CLASS_THERMOSTAT_SETBACK ) {
                if( !value.isKeyExist( "overrideType" ) || !value.isKeyExist( "mode" ) ||
                    !value.isKeyExist( "value" ) ) {
                    ZWAVE_LOGE( "Wrong field" );
                    return;
                }

                auto ccTemp = static_cast<znet::ZCc0x47ThermostatSetback*>( m_ComClass.get() );
                if( ccTemp ) {
                    auto overrideType = ::common::iValueCast<::common::StringValue>( value["overrideType"] );
                    auto mode = ::common::iValueCast<::common::StringValue>( value["mode"] );
                    if( !overrideType ) {
                        ZWAVE_LOGE( "Wrong override type" );
                        return;
                    }
                    if( !mode ) {
                        ZWAVE_LOGE( "Wrong mode" );
                        return;
                    }
                    if( overrideType->get().empty() ) {
                        ZWAVE_LOGE( "Override type is empty" );
                        return;
                    }
                    if( mode->get().empty() ) {
                        ZWAVE_LOGE( "Mode is empty" );
                        return;
                    }
                    auto valuePtr = ::common::iValueCast<::common::FloatValue>( value["value"] );
                    if( !valuePtr ) {
                        m_IsSuccessfull = ccTemp->setSetback(
                            *( znet::thermostat_setback::stringToOverrideType( overrideType->get().c_str() ) ),
                            *( znet::thermostat_setback::stringToMode( mode->get().c_str() ) ) );
                    }
                    else {
                        m_IsSuccessfull = ccTemp->setSetback(
                            *( znet::thermostat_setback::stringToOverrideType( overrideType->get().c_str() ) ),
                            *( znet::thermostat_setback::stringToMode( mode->get().c_str() ) ),
                            znet::thermostat_setback::SetbackValueOpt( valuePtr->get() ) );
                    }
                }
            }
            else if( m_ClassId == ZNET_COMMAND_CLASS_THERMOSTAT_SETPOINT ) {
                if( !value.isKeyExist( "type" ) || !value.isKeyExist( "type" ) || !value.isKeyExist( "scale" ) ) {
                    ZWAVE_LOGE( "Wrong field" );
                    return;
                }

                auto scaleStr = ::common::iValueCast<::common::StringValue>( value["scale"] );
                if( !scaleStr || scaleStr->get().empty() ) {
                    ZWAVE_LOGE( "Wrong scale" );
                    return;
                }

                auto typePtr = ::common::iValueCast<::common::IntValue>( value["type"] );
                auto valuePtr = ::common::iValueCast<::common::FloatValue>( value["value"] );

                if( !typePtr || !valuePtr ) {
                    ZWAVE_LOGE( "Wrong type or value" );
                    return;
                }

                znet::thermostat_setpoint_converter::ScaleResult scale =
                    znet::thermostat_setpoint_converter::scaleFromString( scaleStr->get().c_str() );
                m_IsSuccessfull = scale.second;
                if( m_IsSuccessfull ) {
                    auto ccTemp = static_cast<znet::ZCc0x43ThermostatSetpoint*>( m_ComClass.get() );
                    m_IsSuccessfull = ccTemp->setValue( typePtr->get(), scale.first, valuePtr->get() );
                }
            }
        };
    } visitor( command, cc, classId, channelId );

    command.m_value->accept( visitor );

    // TODO add set for other types (if it is necessary)
    if( !visitor.m_IsSuccessfull ) {
        ZWAVE_LOGE( " Failed a setting of a value" );
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetValueMulticast& command ) {
    if( !command.m_value ) {
        ZWAVE_LOGE( "The item's value is empty " );
        return;
    }

    struct CmdVisitor : common::IValueVisitorConst {
        bus::command::SetValueMulticast& m_Command;
        bool m_IsSuccessfull = false;
        znet::MulticastMode m_mode;

        CmdVisitor( bus::command::SetValueMulticast& command )
        : m_Command( command ) {
            m_mode = *znet::stringToMulticastMode( m_Command.m_mode.c_str() );
        }

        void visit( const common::FloatValue& value ) override {
            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValueMulticast(
                m_Command.m_nodeId.size(),
                m_Command.m_nodeId.data(),
                m_Command.m_channelId,
                m_Command.m_commandClassId,
                static_cast<uint8_t>( value.get() ),
                m_mode );
            m_IsSuccessfull = true;
        };

        void visit( const common::IntValue& value ) override {
            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValueMulticast(
                m_Command.m_nodeId.size(),
                m_Command.m_nodeId.data(),
                m_Command.m_channelId,
                m_Command.m_commandClassId,
                static_cast<uint8_t>( value.get() ),
                m_mode );
            m_IsSuccessfull = true;
        };

        void visit( const common::Int64Value& value ) override {
            visit(::common::IntValue{ static_cast<int>( value.get() ) } );
            m_IsSuccessfull = true;
        };

        void visit( const common::BoolValue& value ) override {
            auto bVal = value.get() ? common::BoolValue::High : common::BoolValue::Low;
            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValueMulticast( m_Command.m_nodeId.size(),
                                                                                          m_Command.m_nodeId.data(),
                                                                                          m_Command.m_channelId,
                                                                                          m_Command.m_commandClassId,
                                                                                          bVal,
                                                                                          m_mode );
            m_IsSuccessfull = true;
        };

        void visit( const common::StringValue& value ) override {
            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValueMulticast(
                m_Command.m_nodeId.size(),
                m_Command.m_nodeId.data(),
                m_Command.m_channelId,
                m_Command.m_commandClassId,
                static_cast<std::string>( value.get() ),
                m_mode );
            m_IsSuccessfull = true;
        };

        void visit( const common::ScalebleValue& value ) override {
            ZWAVE_LOGE( "CmdVisitor: Call not implemented visitor for ScalebleValue!" );
            m_IsSuccessfull = false;
        };

        void visit( const common::TokenValue& value ) override {
            ZWAVE_LOGE( "CmdVisitor: Call not implemented visitor for TokenValue!" );
            m_IsSuccessfull = false;
        };

        void visit( const common::RgbValue& value ) override {
            auto rgbVal = value.get();
            znet::ColorComponents znetColorComponents;
            znetColorComponents.reserve( rgbVal.size() );
            // znet::color_switch_converter::strToColorComponentId();
            for( const common::RgbValue::ColorComponent& component : rgbVal ) {
                znet_cl_component_t znetColorComponent = {
                    static_cast<uint8_t>( rgbValueColorToZnetColor( component.color ) ), component.value
                };
                znetColorComponents.push_back( znetColorComponent );
            }
            gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValueMulticast( m_Command.m_nodeId.size(),
                                                                                          m_Command.m_nodeId.data(),
                                                                                          m_Command.m_channelId,
                                                                                          m_Command.m_commandClassId,
                                                                                          znetColorComponents,
                                                                                          m_mode );

            m_IsSuccessfull = true;
        };

        void visit( const common::NotificationValue& value ) override {
            ZWAVE_LOGE( "CmdVisitor: Call not implemented visitor for NotificationValue!" );
            m_IsSuccessfull = false;
        };

        void visit( const common::UserCodeValue& value ) override {
            ZWAVE_LOGE( "CmdVisitor: Call not implemented visitor for UserCodeValue!" );
            m_IsSuccessfull = false;
        };

        void visit( const common::ArrayValue& value ) override {
            ZWAVE_LOGE( "CmdVisitor: Call not implemented visitor for ArrayValue!" );
            m_IsSuccessfull = false;
        };

        void visit( const common::TableValue& value ) override {
            ZWAVE_LOGE( "CmdVisitor: Call not implemented visitor for TableValue!" );
            m_IsSuccessfull = false;
        };
    } visitor( command );

    command.m_value->accept( visitor );

    // TODO add set for other types (if it is necessary)
    if( !visitor.m_IsSuccessfull ) {
        ZWAVE_LOGE( " Failed a setting of a value" );
    }
    return;
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::SetThermostatMode& command ) {
    if( auto& data = command.manufacturerSpecificData(); data && command.mode() == 0x1F ) {
        if( auto cc = static_cast<znet::ZCc0x40ThermostatMode*>(
                getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_THERMOSTAT_MODE ) ) ) {
            if( auto manufacturerSpecificData = ::znet::utils::hexToBytes( *data ) ) {
                cc->setManufacturerSpecificMode( manufacturerSpecificData->data(), manufacturerSpecificData->size() );
            }
        }
    }
    else {
        gateways::zwave::ZNet::get().getCommandClassValuesSetter().setValue(
            command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_THERMOSTAT_MODE, command.mode() );
    }
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::SwitchLevelChange& command ) {
    if( auto cc = static_cast<znet::ZCc0x26SwitchMultiLevel*>(
            getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_SWITCH_MULTILEVEL ) ) ) {
        if( auto changeCommand = ::znet::stringToValueChangeCommand( command.getCommand().c_str() ) ) {
            cc->setLevelChange( *changeCommand, command.getDuration() ? *command.getDuration() : ::znet::SecondsOpt{} );
        }
    }
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::ColorSwitchLevelChange& command ) {
    if( auto cc = static_cast<znet::ZCc0x33ColorSwitch*>(
            getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_SWITCH_COLOR ) ) ) {
        if( auto changeCommand = ::znet::stringToValueChangeCommand( command.getCommand().c_str() ) ) {
            cc->setLevelChange( rgbValueColorToZnetColor(
                                    ::common::RgbValue::strToColorComponentId( command.getColorComponent().c_str() ) ),
                                *changeCommand,
                                command.getDuration() ? *command.getDuration() : ::znet::SecondsOpt{} );
        }
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RequestValue& command ) {
    m_pollTimer.reset();

    if( command.m_commandClassId == 0 && command.m_channelId == 0 ) {
        if( Gateway::get().getDevicesCount() > 0 ) {
            if( command.m_nodeId == 0 ) {
                gateways::zwave::ZNet::get().requestAllNodesValues();
            }
            else {
                // TODO: does not have time to update user code because of the speed of the lua machine.
                std::chrono::milliseconds requestDelay( 400 );
                if( command.m_requestDelay > requestDelay )
                    requestDelay = command.m_requestDelay;
                gateways::zwave::ZNet::get().requestNodeValues( command.m_nodeId, requestDelay );
            }
        }
        return;
    }

    auto device = Gateway::get().getDevice( command.m_nodeId );
    if( !device ) {
        ZWAVE_LOGE( "RequestValue. Getting of device has been failed. Node's ID is %d", command.m_nodeId );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( command.m_channelId, command.m_commandClassId );
    if( !cc ) {
        ZWAVE_LOGE( "RequestValue. Getting of CC has been failed. Node's ID is %d, CC's ID is 0x%x",
                    command.m_nodeId,
                    command.m_commandClassId );
        return;
    }

    if( !gateways::zwave::ZNet::get().isBusy() ) {
        znet::RequestValue request;
        cc->requestValue( request );
    }
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::RequestValueTermostatSetpoint& command ) {
    auto device = Gateway::get().getDevice( command.m_nodeId );
    if( !device ) {
        ZWAVE_LOGE( "RequestValueTermostatSetpoint. Getting of device has been failed. Node's ID is %d",
                    command.m_nodeId );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( command.m_channelId, command.m_commandClassId );
    if( !cc ) {
        ZWAVE_LOGE( "RequestValueTermostatSetpoint. Getting of CC has been failed. Node's ID is %d, CC's ID is 0x%x",
                    command.m_nodeId,
                    command.m_commandClassId );
        return;
    }

    auto ccTemp = static_cast<znet::ZCc0x43ThermostatSetpoint*>( cc.get() );
    ccTemp->requestSetpoint( command.m_setpointType );
}

void ZNet::visit( const ::bus::BusAddr from,
                  const ::bus::BusAddr to,
                  ::configuration::commands::SetConfiguration& command ) {
    auto device = Gateway::get().getDevice( command.nodeId() );
    if( !device ) {
        ZWAVE_LOGE( "SetConfiguration. Getting of device has been failed. Node's ID is %d", command.nodeId() );
        return;
    }

    znet::ZNode::ClassPtr ccBase = device->getCommandClass( ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION );
    if( !ccBase ) {
        ZWAVE_LOGE( "SetConfiguration. Getting of CC has been failed. Node's ID is %d0x", command.nodeId() );
        return;
    }

    auto cc = static_cast<::znet::ZCc0x70Configuration*>( ccBase.get() );
    if( command.set() ) {
        if( command.paramSize() != 0 ) {
            auto znetConfigParamValue = static_cast<uint32_t>( command.paramValue() );
            if( auto parameter = cc->findV3Parameter( command.paramNumber() ) ) {
                znetConfigParamValue = ::znet::signed_configuration_parameter_converter::toZNetValue(
                    command.paramValue(), command.paramSize(), parameter->m_format );
            }
            cc->setConfigurationValue( command.paramNumber(), command.paramSize(), znetConfigParamValue );
        }
        else {
            cc->resetConfigurationValue( command.paramNumber() );
        }
    }
    else {
        if( !gateways::zwave::ZNet::get().isBusy() ) {
            cc->requestParameterValue( command.paramNumber() );
        }
    }
}

void ZNet::visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::SetBarrierSignal& command ) {
    auto device = Gateway::get().getDevice( command.nodeId() );
    if( !device ) {
        ZWAVE_LOGE( "SetBarrierSignal. Getting of device has been failed. Node's ID is %d", command.nodeId() );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_BARRIER_OPERATOR );
    if( !cc ) {
        ZWAVE_LOGE( "SetBarrierSignal. Getting of CC has been failed. Node's ID is %d0x", command.nodeId() );
        return;
    }

    auto ccTemp = static_cast<znet::ZCc0x66BarrierOperator*>( cc.get() );
    ccTemp->setSignalValue(
        static_cast<uint8_t>( znet::barrier_operator_signal_converter::stringToSignal( command.signalName().c_str() ) ),
        command.value() );
    return;
}

void ZNet::visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::RequestBarrierSignal& command ) {
    auto device = Gateway::get().getDevice( command.nodeId() );
    if( !device ) {
        ZWAVE_LOGE( "RequestBarrierSignal. Getting of device has been failed. Node's ID is %d", command.nodeId() );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_BARRIER_OPERATOR );
    if( !cc ) {
        ZWAVE_LOGE( "RequestBarrierSignal. Getting of CC has been failed. Node's ID is %d0x", command.nodeId() );
        return;
    }

    auto ccTemp = static_cast<znet::ZCc0x66BarrierOperator*>( cc.get() );
    ccTemp->requestSignalValue( static_cast<uint8_t>(
        znet::barrier_operator_signal_converter::stringToSignal( command.signalName().c_str() ) ) );
    return;
}

void ZNet::visit( const ::bus::BusAddr from,
                  const ::bus::BusAddr to,
                  ::configuration::commands::RequestConfiguration& command ) {
    auto device = Gateway::get().getDevice( command.nodeId() );
    if( !device ) {
        ZWAVE_LOGE( "RequestConfiguration. Getting of device has been failed. Node's ID is %d", command.nodeId() );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION );
    if( !cc ) {
        ZWAVE_LOGE( "RequestConfiguration. Getting of CC has been failed. Node's ID is %d0x", command.nodeId() );
        return;
    }

    auto ccTemp = static_cast<znet::ZCc0x70Configuration*>( cc.get() );
    ccTemp->requestParameterValue( command.paramNumber() );

    return;
}

void ZNet::visit( const ::bus::BusAddr from,
                  const ::bus::BusAddr to,
                  ::configuration::commands::AddConfiguration& command ) {
    auto device = Gateway::get().getDevice( command.nodeId() );
    if( !device ) {
        ZWAVE_LOGE( "AddConfiguration. Getting of device has been failed. Node's ID is %d", command.nodeId() );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION );
    if( !cc ) {
        ZWAVE_LOGE( "AddConfiguration. Getting of CC has been failed. Node's ID is %d0x", command.nodeId() );
        return;
    }

    auto ccTemp = static_cast<znet::ZCc0x70Configuration*>( cc.get() );
    ccTemp->addConfiguration( command.paramNumber(), command.paramSize() );

    return;
}

void ZNet::visit( const ::bus::BusAddr from,
                  const ::bus::BusAddr to,
                  ::configuration::commands::RemoveConfiguration& command ) {
    auto device = Gateway::get().getDevice( command.nodeId() );
    if( !device ) {
        ZWAVE_LOGE( "RemoveConfiguration. Getting of device has been failed. Node's ID is %d", command.nodeId() );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION );
    if( !cc ) {
        ZWAVE_LOGE( "RemoveConfiguration. Getting of CC has been failed. Node's ID is %d0x", command.nodeId() );
        return;
    }

    auto ccTemp = static_cast<znet::ZCc0x70Configuration*>( cc.get() );
    ccTemp->removeConfiguration( command.paramNumber() );

    return;
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::configuration::commands::SetDoorLockConfiguration& command ) {
    if( auto cc = static_cast<znet::ZCc0x62DoorLock*>(
            getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_DOOR_LOCK ) ) ) {
        std::optional<znet_door_lock_extended_configuration_t> extConf;
        if( auto extConfData = command.getExtendedConfiguration() ) {
            extConf = { .auto_relock_timeout = extConfData->m_autoRelockTimeout,
                        .hold_and_release_timeout = extConfData->m_holdAndReleaseTimeout,
                        .twist_assist_enabled = extConfData->m_twistAssistEnabled,
                        .block_to_block_enabled = extConfData->m_blockToBlockEnabled };
        }

        znet_door_lock_configuration_t conf = { 0, 0, 0, false };

        if( ( conf.are_timed_operations_enabled = command.getLockTimeout().has_value() ) ) {
            conf.lock_timeout = *command.getLockTimeout();
        }

        struct {
            void operator()( uint8_t value ) {
                if( value >= 1 && value <= 4 ) {
                    m_result |= 1 << ( value - 1 );
                }
            }
            uint8_t& m_result;
        } parseInsideIds{ conf.enabled_inside_handle_ids }, parseOutsideIds{ conf.enabled_outside_handle_ids };

        std::for_each( command.getInsideHandleIds().begin(), command.getInsideHandleIds().end(), parseInsideIds );
        std::for_each( command.getOutsideHandleIds().begin(), command.getOutsideHandleIds().end(), parseOutsideIds );

        cc->setConfiguration( conf, extConf ? &*extConf : nullptr );
    }
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::configuration::commands::RequestDoorLockConfiguration& command ) {
    using namespace std::placeholders;
    if( auto cc = static_cast<znet::ZCc0x62DoorLock*>(
            getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_DOOR_LOCK ) ) ) {
        cc->requestConfiguration();
    }
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::configuration::commands::ResetAllConfigurationValues& cmd ) {
    using namespace std::placeholders;
    if( auto cc = static_cast<znet::ZCc0x70Configuration*>(
            getCommandClass( cmd.getNodeId(), ZNET_CHANNEL_ID_ROOT, ZNET_COMMAND_CLASS_CONFIGURATION ) ) ) {
        cc->resetAllConfigurationValues();
    }
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RequestDoorLockState& command ) {
    using namespace std::placeholders;
    if( auto cc = static_cast<znet::ZCc0x62DoorLock*>(
            getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_DOOR_LOCK ) ) ) {
        cc->requestOperation();
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetUserCode& command ) {
    auto device = Gateway::get().getDevice( command.m_nodeId );
    if( !device ) {
        ZWAVE_LOGE( "UserCode. Getting of device has been failed. Node's ID is %d", command.m_nodeId );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( command.m_channelId, ZNET_COMMAND_CLASS_USER_CODE );
    if( !cc ) {
        ZWAVE_LOGE( "UserCode. Getting of CC has been failed. Node's ID is %d0x", command.m_nodeId );
        return;
    }

    auto ccTemp = static_cast<znet::ZCc0x63UserCode*>( cc.get() );
    auto status = znet::user_code_converter::userIdStatusFromString( command.m_user_id_status.c_str() );
    ccTemp->setUserCode( command.m_user_id, static_cast<uint8_t>( *status ), command.m_user_code );

    return;
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RequestUserCode& command ) {
    auto device = Gateway::get().getDevice( command.m_nodeId );
    if( !device ) {
        ZWAVE_LOGE( "UserCode. Getting of device has been failed. Node's ID is %d", command.m_nodeId );
        return;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( command.m_channelId, ZNET_COMMAND_CLASS_USER_CODE );
    if( !cc ) {
        ZWAVE_LOGE( "UserCode. Getting of CC has been failed. Node's ID is %d0x", command.m_nodeId );
        return;
    }

    znet::RequestValueUserCode request;
    request.userId = command.m_user_id;
    cc->requestValue( request );
    return;
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::UserCodeV2Operation& command ) {
    if( auto cc = static_cast<znet::ZCc0x63UserCode*>(
            getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_USER_CODE ) ) ) {
        switch( command.getType() ) {
        case ::bus::command::UserCodeV2Operation::REQUEST_MASTER_CODE:
            cc->requestMasterCode();
            break;
        case ::bus::command::UserCodeV2Operation::REQUEST_KEYPAD_MODE:
            cc->requestKeypadMode();
            break;
        case ::bus::command::UserCodeV2Operation::REQUEST_CHECKSUM:
            cc->requestChecksum();
            break;
        case ::bus::command::UserCodeV2Operation::SET_MASTER_CODE:
            if( auto code = command.getArg() ) {
                cc->setMasterCode( *code );
            }
            break;
        case ::bus::command::UserCodeV2Operation::SET_KEYPAD_MODE:
            if( auto modeStr = command.getArg() ) {
                if( auto mode = ::znet::user_code_converter::keypadModeFromString( modeStr->c_str() ) ) {
                    cc->setKeypadMode( *mode );
                }
            }
            break;
        }
    }
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::SetDailyRepeatingEntryLockSchedule& command ) {
    if( auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
            getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) ) {
        const ::common::TableValue& value = command.value();
        namespace tokens = ::common::resources::lua_events;
        using ::common::iValueCast;
        auto weekDays = iValueCast<::common::ArrayValue>( value[tokens::WeekDays] );
        if( !weekDays ) {
            DAILY_LOGE( "%s element is required to set daily repeating entry lock schedule", tokens::WeekDays );
            return;
        }
        znet_daily_repeating_schedule_t schedule = {
            .schedule_id = { .user_id = command.userId(), .slot_id = command.slotId() },
            .week_day_bitmask = weekDaysFromArrayOfStrings( *weekDays ),
            .start_hour = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartHour] )->get() ),
            .start_min = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartMin] )->get() ),
            .duration_hour =
                static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::DurationHour] )->get() ),
            .duration_min = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::DurationMin] )->get() )
        };
        if(::is_daily_repeating_valid( &schedule ) ) {
            if( cc->setDailyRepeatingSchedule( schedule ) ) {
                DAILY_LOGD( "Set done" );
            }
            else {
                DAILY_LOGE( "Set failure" );
            }
        }
        else {
            DAILY_LOGE( "Invalid schedule" );
        }
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetWeekDayEntryLockSchedule& command ) {
    if( auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
            getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) ) {
        const ::common::TableValue& value = command.value();
        namespace tokens = ::common::resources::lua_events;
        using ::common::iValueCast;
        auto weekDayStr = iValueCast<::common::StringValue>( value[tokens::WeekDay] );
        if( !weekDayStr ) {
            WEEK_DAY_LOGE( "%s element is required to set week day entry lock schedule", tokens::WeekDay );
            return;
        }
        auto weekDay = ::znet::getWeekDayByName( weekDayStr->get() );
        if( !weekDay ) {
            WEEK_DAY_LOGE( "Cannot convert week day name: %s", weekDayStr->get().c_str() );
            return;
        }
        ::znet_week_day_schedule_t schedule = {
            .schedule_id = { .user_id = command.userId(), .slot_id = command.slotId() },
            .day_of_week = static_cast<uint8_t>( *weekDay ),
            .start_hour = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartHour] )->get() ),
            .start_min = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartMin] )->get() ),
            .stop_hour = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StopHour] )->get() ),
            .stop_min = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StopMin] )->get() )
        };
        if(::is_week_day_schedule_valid( &schedule ) ) {
            if( cc->setWeekDaySchedule( schedule ) ) {
                WEEK_DAY_LOGD( "Set done" );
            }
            else {
                WEEK_DAY_LOGE( "Set failure" );
            }
        }
        else {
            WEEK_DAY_LOGE( "Invalid schedule" );
        }
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetYearDayEntryLockSchedule& command ) {
    if( auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
            getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) ) ) {
        const ::common::TableValue& value = command.value();
        namespace tokens = ::common::resources::lua_events;
        using ::common::iValueCast;
        znet_year_day_schedule_t schedule = {
            .schedule_id = { .user_id = command.userId(), .slot_id = command.slotId() },
            .start_year =
                static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartYear] )->get() - 2000 ),
            .start_month = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartMonth] )->get() ),
            .start_day = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartDay] )->get() ),
            .start_hour = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartHour] )->get() ),
            .start_minute = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StartMin] )->get() ),
            .stop_year =
                static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StopYear] )->get() - 2000 ),
            .stop_month = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StopMonth] )->get() ),
            .stop_day = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StopDay] )->get() ),
            .stop_hour = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StopHour] )->get() ),
            .stop_minute = static_cast<uint8_t>( iValueCast<::common::IntValue>( value[tokens::StopMin] )->get() )
        };
        if(::is_year_day_schedule_valid( &schedule ) ) {
            if( cc->setYearDaySchedule( schedule ) ) {
                YEAR_DAY_LOGD( "Set done" );
            }
            else {
                YEAR_DAY_LOGE( "Set failure" );
            }
        }
        else {
            YEAR_DAY_LOGE( "Invalid schedule" );
        }
    }
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::RemoveDailyRepeatingEntryLockSchedule& command ) {
    auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
        getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) );
    if( !cc || !cc->removeDailyRepeatingSchedule( { .user_id = command.userId(), .slot_id = command.slotId() } ) ) {
        DAILY_LOGE( "Remove failure" );
    }
    else {
        DAILY_LOGD( "Remove done" );
    }
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::RemoveWeekDayEntryLockSchedule& command ) {
    auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
        getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) );
    if( !cc || !cc->removeWeekDaySchedule( { .user_id = command.userId(), .slot_id = command.slotId() } ) ) {
        WEEK_DAY_LOGE( "Remove failure" );
    }
    else {
        WEEK_DAY_LOGD( "Remove done" );
    }
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::RemoveYearDayEntryLockSchedule& command ) {
    auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
        getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) );
    if( !cc || !cc->removeYearDaySchedule( { .user_id = command.userId(), .slot_id = command.slotId() } ) ) {
        YEAR_DAY_LOGE( "Remove failure" );
    }
    else {
        YEAR_DAY_LOGD( "Remove done" );
    }
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::RequestDailyRepeatingEntryLockSchedule& command ) {
    auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
        getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) );
    if( !cc || !cc->requestDailyRepeatingSchedule( { .user_id = command.userId(), .slot_id = command.slotId() } ) ) {
        DAILY_LOGE( "Request failure" );
    }
    else {
        DAILY_LOGD( "Request done" );
    }
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::RequestWeekDayEntryLockSchedule& command ) {
    auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
        getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) );
    if( !cc || !cc->requestWeekDaySchedule( { .user_id = command.userId(), .slot_id = command.slotId() } ) ) {
        WEEK_DAY_LOGE( "Request failure" );
    }
    else {
        WEEK_DAY_LOGD( "Request done" );
    }
}

void ZNet::visit( const bus::BusAddr from,
                  const bus::BusAddr to,
                  bus::command::RequestYearDayEntryLockSchedule& command ) {
    auto cc = static_cast<znet::ZCc0x4EScheduleEntryLock*>(
        getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK ) );
    if( !cc || !cc->requestYearDaySchedule( { .user_id = command.userId(), .slot_id = command.slotId() } ) ) {
        YEAR_DAY_LOGE( "Request failure" );
    }
    else {
        YEAR_DAY_LOGD( "Request done" );
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SoundSwitchConfigrationSet& command ) {
    auto cc = static_cast<znet::ZCc0x79SoundSwitch*>(
        getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_SOUND_SWITCH ) );
    if( cc && cc->setConfiguration( command.getVolume(), command.getToneId() ) ) {
        ESP_LOGD(::esp_log_tags::ZNET_SOUND_SWITCH_CONFIG, "Set done" );
    }
    else {
        ESP_LOGE(::esp_log_tags::ZNET_SOUND_SWITCH_CONFIG, "Set failure" );
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SoundSwitchConfigrationGet& command ) {
    auto cc = static_cast<znet::ZCc0x79SoundSwitch*>(
        getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_SOUND_SWITCH ) );
    if( cc && cc->requestConfiguration() ) {
        ESP_LOGD(::esp_log_tags::ZNET_SOUND_SWITCH_CONFIG, "Request done" );
    }
    else {
        ESP_LOGE(::esp_log_tags::ZNET_SOUND_SWITCH_CONFIG, "Request failure" );
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SoundSwitchTonePlaySet& command ) {
    auto cc = static_cast<znet::ZCc0x79SoundSwitch*>(
        getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_SOUND_SWITCH ) );
    if( cc && cc->setTonePlay( command.getToneId(), command.getVolume() ) ) {
        ESP_LOGD(::esp_log_tags::ZNET_SOUND_SWITCH_TONE, "Set done" );
    }
    else {
        ESP_LOGE(::esp_log_tags::ZNET_SOUND_SWITCH_TONE, "Set failure" );
    }
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SoundSwitchTonePlayGet& command ) {
    auto cc = static_cast<znet::ZCc0x79SoundSwitch*>(
        getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_SOUND_SWITCH ) );
    if( cc && cc->requestTonePlay() ) {
        ESP_LOGD(::esp_log_tags::ZNET_SOUND_SWITCH_TONE, "Request done" );
    }
    else {
        ESP_LOGE(::esp_log_tags::ZNET_SOUND_SWITCH_TONE, "Request failure" );
    }
}

void ZNet::visit( bus::BusAddr from, bus::BusAddr to, bus::command::GpioButtonClicked& command ) {
    sendNif();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningAdd& cmd ) {
    auto result = getProvisioningListController().addProvisioningListEntry( cmd.getDsk(), cmd.getFields() );
    using OperationStatus = ::znet::api::IProvisioningListController::OperationStatus;
    cmd.setResponce( OperationStatus::ENTRY_REPLACED == result ? OperationStatus::SUCCESS : result );
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningRemove& command ) {
    for( auto id : command.getProvisioningIdList() ) {
        getProvisioningListController().removeProvisioningListEntry( id );
    }
    command.setResponce(::znet::api::IProvisioningListController::OperationStatus::SUCCESS );
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningList& cmd ) {
    using OperationStatus = ::znet::api::IProvisioningListController::OperationStatus;
    OperationStatus result;
    if( !cmd.getNodeIdList().empty() || !cmd.getProvisioningIdList().empty() ) {
        ::znet::api::ProvisioningListQuery{ getProvisioningListControllerDataSource() }
            .filterBy( fields::nodeId( cmd.getNodeIdList() ) || fields::provisioningId( cmd.getProvisioningIdList() ) )
            .tap( cmd.getProcessor() )
            .execute( result );
    }
    else {
        result = getProvisioningListController().findProvisioningListEntry( cmd.getProcessor() );
    }
    cmd.setResponce( OperationStatus::ENTRY_NOT_FOUND == result ? OperationStatus::SUCCESS : result );
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningUpdate& command ) {
    auto query = ::znet::api::ProvisioningListQuery{ getProvisioningListControllerDataSource() };
    query.filterBy( fields::nodeId( command.getNodeId() ) );
    if( auto newName = command.getName() ) {
        query.setField( &::znet::ProvisioningListEntryFields::m_name, *newName );
    }
    if( auto newRoom = command.getRoom() ) {
        query.setField( &::znet::ProvisioningListEntryFields::m_room, *newRoom );
    }
    query.execute();
}

void ZNet::visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningGet& command ) {
    commands::ZwaveProvisioningGetResponse response;
    ::znet::api::ProvisioningListQuery{ getProvisioningListControllerDataSource() }
        .filterBy( fields::nodeId( command.getNodeId() ) )
        .getField( &::znet::ProvisioningListEntryFields::m_name, response.m_name, {} )
        .getField( &::znet::ProvisioningListEntryFields::m_room, response.m_room, {} )
        .execute();
    command.setResponce( response );
}

void ZNet::visit( bus::BusAddr, bus::BusAddr, ::bus::command::SendIdentify& command ) {
    if( auto cc = static_cast<znet::ZCc0x87Indicator*>(
            getCommandClass( command.getNodeId(), command.getChannelId(), ZNET_COMMAND_CLASS_INDICATOR ) ) ) {
        cc->sendIdentifyCommand();
    }
}

void ZNet::visit( bus::BusAddr, bus::BusAddr, ::bus::command::UpdatesNodesFirmware& command ) {
    common::DeviceFirmwareUpdateManager::Devices node_ids = command.node_ids();
    const auto& items = command.firmware();

    common::DeviceFirmwareUpdateManager::TargetsToLinks targetsToLinks;
    for( auto& item : items ) {
        auto firmwareJson = item->toJson( "firmware" );

        char* fwJson = const_cast<char*>( firmwareJson.c_str() );
        gason::JsonValue json;
        std::unique_ptr<gason::JsonAllocator> jsonAlloc = std::make_unique<gason::JsonAllocator>();
        if( gason::JSON_PARSE_OK == gason::jsonParse( fwJson, json, *jsonAlloc ) ) {
            {
                auto firmware = json( "firmware" );
                if( firmware ) {
                    auto image = firmware( "image" ).toString();

                    if( !image ) {
                        ZWAVE_LOGW( "'image' not found" );
                    }

                    auto checksum = firmware( "checksum" ).toString();
                    if( !checksum ) {
                        ZWAVE_LOGW( "'checksum' not found" );
                    }

                    auto target = firmware( "target" ).toInt();
                    if( !target ) {
                        ZWAVE_LOGW( "'target' not found" );
                    }

                    common::DeviceFirmwareUpdateManager::ExtendedLink link{ image, checksum };
                    targetsToLinks.emplace( target, std::move( link ) );
                }
            }
        }
    }

    m_deviceFirmwareUpdateManager->addUpdateScope( std::move( node_ids ), std::move( targetsToLinks ) );
}

void ZNet::visit( bus::BusAddr, bus::BusAddr, ::bus::command::TimezoneWasUpdated& ) {
    gateways::zwave::ZNet::get().updateTimeSettings();
}

void ZNet::visit( const ::bus::BusAddr from,
                  const ::bus::BusAddr to,
                  ::bus::command::SetDevicePollIntervals& command ) {
    gateways::zwave::ZNet::get().updateReachableCheckTimeoutSettings( command.useFixedWakeUpInterval(),
                                                                      command.wakeUpInterval(),
                                                                      command.alwaysOnDevicesInetrval(),
                                                                      command.attemptsCount() );
}

void ZNet::visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::CheckNodeValues& command ) {
    const auto& ccToCheck = command.cc_to_check();

    std::vector<std::pair<uint8_t, uint8_t>> dataToCheck;
    for( auto& pair : ccToCheck ) {
        auto table = common::iValueCast<common::TableValue>( pair.get() );
        uint8_t channel_id = ( common::iValueCast<common::IntValue>( ( *table )["channel_id"] ) )->get();
        uint8_t class_id = ( common::iValueCast<common::IntValue>( ( *table )["class_id"] ) )->get();
        dataToCheck.push_back( std::make_pair( channel_id, class_id ) );
    }
    gateways::zwave::ZNet::get().checkNodeValues( command.node_id(), command.request_delay(), dataToCheck );
}

void ZNet::visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::CheckReachableStatus& command ) {
    gateways::zwave::ZNet::get().checkReachableStatus( command.node_id() );
}

void ZNet::visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::LockAntiTheft& command ) {
    if( auto cc = static_cast<znet::ZCc0x5DAntiTheft*>(
            getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_ANTITHEFT ) ) ) {
        cc->lock( command.magicCode(), command.hint(), getManufacturerId(), getZWaveAllianceLockingId() );
    }
}

void ZNet::visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::LockAntiTheftDefault& command ) {
    if( auto cc = static_cast<znet::ZCc0x5DAntiTheft*>(
            getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_ANTITHEFT ) ) ) {
        const auto hint = generateHint();
        const auto magicCode = getMagicCode( hint );
        if( !magicCode ) {
            ZWAVE_LOGE( "Cannot generate magic code from hint" );
            return;
        }
        cc->lock( *magicCode, hint, getManufacturerId(), getZWaveAllianceLockingId() );
    }
}

void ZNet::visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::UnlockAntiTheft& command ) {
    const uint8_t nodeId = command.nodeId();
    const uint8_t channelId = command.channelId();
    const uint8_t classId = command.classId();
    const auto magicCode = command.magicCode();
    if( classId == ZNET_COMMAND_CLASS_ANTITHEFT ) {
        if( auto cc = static_cast<znet::ZCc0x5DAntiTheft*>(
                getCommandClass( nodeId, channelId, ZNET_COMMAND_CLASS_ANTITHEFT ) ) ) {
            cc->unlock( magicCode );
        }
    }
    else {
        if( classId == ZNET_COMMAND_CLASS_ANTITHEFT_UNLOCK ) {
            if( auto cc = static_cast<znet::ZCc0x7EAntiTheftUnlock*>(
                    getCommandClass( nodeId, channelId, ZNET_COMMAND_CLASS_ANTITHEFT_UNLOCK ) ) ) {
                cc->unlock( magicCode );
            }
        }
    }
}

void ZNet::visit( const ::bus::BusAddr from,
                  const ::bus::BusAddr to,
                  ::bus::command::UnlockAntiTheftDefault& command ) {
    const uint8_t nodeId = command.nodeId();
    const uint8_t channelId = command.channelId();
    const uint8_t classId = command.classId();
    const auto hint = znet::utils::hexToBytes( command.hint() );
    if( !hint ) {
        ZWAVE_LOGE( "Wrong hint" );
        return;
    }

    const auto magicCode = getMagicCode( *hint );
    if( !magicCode ) {
        ZWAVE_LOGE( "Cannot generate magic code from hint" );
        return;
    }

    if( classId == ZNET_COMMAND_CLASS_ANTITHEFT ) {
        if( auto cc = static_cast<znet::ZCc0x5DAntiTheft*>(
                getCommandClass( nodeId, channelId, ZNET_COMMAND_CLASS_ANTITHEFT ) ) ) {
            cc->unlock( *magicCode );
        }
    }
    else {
        if( classId == ZNET_COMMAND_CLASS_ANTITHEFT_UNLOCK ) {
            if( auto cc = static_cast<znet::ZCc0x7EAntiTheftUnlock*>(
                    getCommandClass( nodeId, channelId, ZNET_COMMAND_CLASS_ANTITHEFT_UNLOCK ) ) ) {
                cc->unlock( *magicCode );
            }
        }
    }
}

void ZNet::visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RequestAlarmV1Value& command ) {
    if( auto cc = static_cast<znet::ZCc0x71Notification*>(
            getCommandClass( command.nodeId(), command.channelId(), ZNET_COMMAND_CLASS_NOTIFICATION ) ) ) {
        cc->requestV1AlarmValue( command.alarmType() );
    }
}

znet::INodesProvider::NodeIdsSet ZNet::getNodeIds() const {
    znet::INodesProvider::NodeIdsSet nodes;

    for( size_t m_deviceIdx = 0; m_deviceIdx < gateways::zwave::Gateway::get().getDevicesCount(); ++m_deviceIdx ) {
        auto device = gateways::zwave::Gateway::get().getDeviceByIndex( m_deviceIdx );

        znet::utils::setBit( nodes.mask, device->getId() );
    }
    return nodes;
}

std::unique_ptr<znet::INodeSetting> ZNet::provideNodeSetting( uint16_t manufacturerId,
                                                              uint16_t productType,
                                                              uint16_t productId ) {
    ZWAVE_LOGD( "path %s, manufacturerId %d, productType %d, productId %d",
                m_deviceSettingsFilePath.c_str(),
                manufacturerId,
                productType,
                productId );
    return std::make_unique<NodeSetting>( m_deviceSettingsFilePath, manufacturerId, productType, productId );
}

uint8_t ZNet::weekDaysFromArrayOfStrings( const common::ArrayValue& arrayOfWeekDayStrings ) {
    std::vector<std::string> convertedArray;
    std::transform( arrayOfWeekDayStrings.begin(),
                    arrayOfWeekDayStrings.end(),
                    std::back_inserter( convertedArray ),
                    []( const std::unique_ptr<::common::IValue>& value ) {
                        if( auto strValue = ::common::iValueCast<::common::StringValue>( value.get() ) ) {
                            return strValue->get();
                        }
                        else {
                            return std::string{};
                        }
                    } );

    return znet::getWeekDaysMask( convertedArray.begin(), convertedArray.end(), []( const std::string& weekDayName ) {
        ESP_LOGE( "weekDaysFromArrayOfStrings", "Unknown week day name: %s", weekDayName.c_str() );
    } );
}

std::vector<uint8_t> ZNet::parseDoorLockHandleIds( uint8_t bitset ) {
    std::vector<uint8_t> result;
    znet::utils::forEachBit( &bitset, 1, [&result]( uint8_t index ) { result.push_back( index + 1 ); } );
    return result;
}

std::shared_ptr<::gateways::zwave::devices::Device> ZNet::getDeviceAndSetReachable( uint8_t nodeId ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        ZWAVE_LOGE( "Gateway::getDevice( nodeId=%d ) returned null", nodeId );
        return nullptr;
    }
    device->setReachable(::znet::ReachableState::Reachable );
    return device;
}

znet::ZClass* ZNet::getCommandClass( uint8_t nodeId, uint8_t channelId, uint8_t commandClassId ) {
    auto device = Gateway::get().getDevice( nodeId );
    if( !device ) {
        ZWAVE_LOGE( "Gateway::getDevice( nodeId=%d ) returned null", nodeId );
        return nullptr;
    }

    znet::ZNode::ClassPtr cc = device->getCommandClass( channelId, commandClassId );
    if( !cc ) {
        ZWAVE_LOGE(
            "Cannot get command class nodeId=%d channelId=%d, commandClassId=%d", nodeId, channelId, commandClassId );
        return nullptr;
    }

    return cc.get();
}

void ZNet::notify( std::function<std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>( std::string )> func ) {
    for( auto entry : m_subscribers ) {
        for( auto name : entry.second ) {
            send( entry.first, func( ( name ) ) );
        }
    }
}

::znet::ColorComponentID ZNet::rgbValueColorToZnetColor(::common::RgbValue::Color rgbColor ) {
    const std::map<::common::RgbValue::Color, ::znet::ColorComponentID> RgbColorToZnetColorId = {
        { ::common::RgbValue::Color::Wwhite, ::znet::ColorComponentID::Wwhite },
        { ::common::RgbValue::Color::Cwhite, ::znet::ColorComponentID::Cwhite },
        { ::common::RgbValue::Color::Red, ::znet::ColorComponentID::Red },
        { ::common::RgbValue::Color::Green, ::znet::ColorComponentID::Green },
        { ::common::RgbValue::Color::Blue, ::znet::ColorComponentID::Blue },
        { ::common::RgbValue::Color::Amber, ::znet::ColorComponentID::Amber },
        { ::common::RgbValue::Color::Cyan, ::znet::ColorComponentID::Cyan },
        { ::common::RgbValue::Color::Purple, ::znet::ColorComponentID::Purple },
        { ::common::RgbValue::Color::Indexed, ::znet::ColorComponentID::Indexed }
    };
    return RgbColorToZnetColorId.at( rgbColor );
}

}  // namespace zwave
}  // namespace gateways
