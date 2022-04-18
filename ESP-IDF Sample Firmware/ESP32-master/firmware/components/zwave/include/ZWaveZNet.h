
#pragma once

#include <znet/znet.hpp>
#include <driver/uart.h>
#include <algorithm>
#include <Bus.h>
#include <BusCommandSetHandler.h>
#include <znet/enums/multilevel_sensor.hpp>
#include <znet/enums/user_code.hpp>
#include <znet/znet_notification.hpp>
#include <znet/enums/door_lock.hpp>
#include <znet/enums/user_code.hpp>
#include <znet/znet_alarm_sensor.hpp>
#include <znet/znet_default_date_time_provider.hpp>
#include <znet/znet_default_timer_provider.hpp>
#include <znet/znet_requests_manager.hpp>
#include <znet/znet_request_status.hpp>
#include <IGateway.h>
#include <znet/iznet_node_setting.hpp>
#include <znet/iznet_provisioning_list_observer.hpp>
#include <znet/znet_reachable_check_timeout_settings.hpp>

#include <DefaultDeviceFirmwareUpdateDelegate.h>
#include "DeviceFirmwareUpdateManager.h"
#include "Publisher.h"

#include <CommonResources.h>
#include <values/RgbValue.h>

#include "Thread.h"

namespace common {
class ArrayValue;
}
namespace znet {
class IProvisioningListProvider;
}

namespace gateways::zwave::devices {
class Device;
}

namespace gateways {
namespace zwave {

class ZNet : public znet::ZNet,
             public znet::DefaultDateTimeProvider,
             public znet::DefaultTimerProvider,
             public bus::IBusClient,
             public znet::IProvisioningListObserver,
             public common::DefaultDeviceFirmwareUpdateDelegate,
             public Publisher {
    static constexpr auto BUS_CLIENT_NAME = "znet";
    static constexpr uint16_t UartBufSize = std::max( 512, UART_FIFO_LEN );
    static constexpr auto STORE_PATH = "/vol/znet.dat";  // see storage::Volume::BASE_PATH from Volume.h
    static constexpr int BUS_WAIT_TIME_MS = 0;
    static constexpr int ITEMS_ADD_DELAY_BRODCAST = 100;

    static constexpr const char* broadcasts[5][7] = { { common::resources::broadcasts::events::IncludeInvoked,
                                                        common::resources::broadcasts::events::IncludeStarted,
                                                        common::resources::broadcasts::events::IncludeFinished,
                                                        common::resources::broadcasts::events::InProgress,
                                                        common::resources::broadcasts::events::IncludeFinishedError,
                                                        common::resources::broadcasts::events::IncludeTimeout,
                                                        common::resources::broadcasts::events::Undefined },
                                                      { common::resources::broadcasts::events::ExcludeInvoked,
                                                        common::resources::broadcasts::events::ExcludeStarted,
                                                        common::resources::broadcasts::events::ExcludeFinished,
                                                        common::resources::broadcasts::events::InProgress,
                                                        common::resources::broadcasts::events::ExcludeFinishedError,
                                                        common::resources::broadcasts::events::ExcludeTimeout,
                                                        common::resources::broadcasts::events::Undefined },
                                                      { common::resources::broadcasts::events::LearnInvoked,
                                                        common::resources::broadcasts::events::LearnStarted,
                                                        common::resources::broadcasts::events::LearnFinished,
                                                        common::resources::broadcasts::events::LearnProgress,
                                                        common::resources::broadcasts::events::LearnFinishedError,
                                                        common::resources::broadcasts::events::LearnTimeout,
                                                        common::resources::broadcasts::events::Undefined },
                                                      { common::resources::broadcasts::events::Undefined,
                                                        common::resources::broadcasts::events::ReconstructStarted,
                                                        common::resources::broadcasts::events::ReconstructFinished,
                                                        common::resources::broadcasts::events::ReconstructProgress,
                                                        common::resources::broadcasts::events::ReconstructFinishedError,
                                                        common::resources::broadcasts::events::Undefined,
                                                        common::resources::broadcasts::events::Undefined },
                                                      { common::resources::broadcasts::events::Invoked,
                                                        common::resources::broadcasts::events::Started,
                                                        common::resources::broadcasts::events::Finished,
                                                        common::resources::broadcasts::events::Undefined,
                                                        common::resources::broadcasts::events::Failed,
                                                        common::resources::broadcasts::events::Timeout,
                                                        common::resources::broadcasts::events::Undefined } };

public:
    static ZNet& get();

    void startup( gateways::IGateway::ScriptExecutor scriptExecutor );

    std::string getUpdateImageName();

public:
    inline const auto& isZnetInitCompleted() const {
        return m_isZnetInitCompleted;
    }

    bool onLoop() override;

    znet::ZNode* onFindNode( znet_node_id_t nodeId ) override;

public:
    void znetLog( int lvl, const char* fmt, va_list list, void* arg ) override;
    void* znetAlloc( void* ptr, size_t size, void* arg ) override;
    uint32_t znetClock( void* arg ) override;

    bool uartInit() override;
    void uartClose() override;
    int znetUartWrite( const void* data, size_t size, size_t* ret_size ) override;
    int znetUartRead( void* data, size_t size, size_t* ret_size ) override;
    int znetStoreSave( size_t offset, const void* data, size_t size ) override;
    int znetStoreLoad( size_t offset, void* data, size_t buffLen ) override;
    int znetStoreReset() override;

    void znetS2SelectKeys( znet_S2_keys_t keyList, void* arg ) override;
    void znetS2RequestPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) override;
    void znetS2ShowPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) override;

    void testUpdatesNodesFirmware();
    void test_visit();

    void zwaveModuleResetPinRelease() override;
    void zwaveModuleResetPinAssert() override;

    bool getKnownS0Key( uint8_t* ) override;

    void nodeCreate( znet_node_id_t nodeId ) override;
    void nodeRemove( const uint8_t nodeId ) override;
    void interviewDone( const uint8_t nodeId ) override;
    void onCheckReachable( uint8_t nodeId = 0 ) override;  // TOD  do we need this?

    void onUpdateReachable( uint8_t nodeId, bool reachable ) override;

    void onUpdateState(::znet::IZNet::Mode cmd, ::znet::ModeStatus cmdStatus, ::znet::ErrorOpt error ) override;

    void onDeviceIncludeProgressUpdate( uint8_t progress ) override;

    void onLearnProgressUpdate( uint8_t devicesAmount, uint8_t currentDeviceIndex ) override;

    void onReconstructNetworkUpdate( const uint8_t devicesAmount, const uint8_t currentDeviceIndex ) override;

    void onReconstructNetworkError( const znet_node_id_t deviceId ) override;

    virtual void onUpdateValue( const uint8_t nodeId,
                                const uint8_t command,
                                const uint8_t channelId,
                                const uint8_t currentValue,
                                const znet::Uint8Opt targetValue,
                                const znet::SecondsOpt targetDuration ) override;

    void onUpdateTokenValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const std::string& value ) override;

    void onUpdateValueAlarmSensor( const uint8_t nodeId,
                                   const uint8_t channelId,
                                   const znet::AlarmSensorType sensorType,
                                   const uint8_t value,
                                   const uint16_t seconds ) override;

    void onUpdateValueBinarySensor( const uint8_t nodeId,
                                    const uint8_t command,
                                    const uint8_t channelId,
                                    const uint8_t sensorType,
                                    const uint8_t value ) override;

    void onUpdateValueMultilevelSensor( const uint8_t nodeId,
                                        const uint8_t ccId,
                                        const uint8_t channelId,
                                        const znet::MultilevelSensor::Type type,
                                        const uint8_t scale,
                                        const float value ) override;

    void onUpdateValueDoorLockOperation( const uint8_t nodeId,
                                         const uint8_t channelId,
                                         const znet::DoorLock::Mode mode,
                                         const uint8_t insideHandlesMode,
                                         const uint8_t outsideHandlesMode,
                                         const znet::DoorCondition condition,
                                         const znet::SecondsOpt lockTimeout,
                                         const znet::DoorLock::ModeOpt targetMode,
                                         const znet::SecondsOpt targetModeTimeout ) override;

    void onUpdateValueDoorLockConfiguration( const uint8_t nodeId,
                                             const uint8_t channelId,
                                             const znet_door_lock_configuration_t& config,
                                             const znet_door_lock_extended_configuration_t* extConfig ) override;

    void onUpdateThermostatModeValue( const uint8_t nodeId,
                                      const uint8_t channelId,
                                      const uint8_t mode,
                                      ::znet::ManufacturerSpecificDataOpt manufacturerSpecificData ) override;

    void onUpdateValueThermostatSetpoint( const uint8_t nodeId,
                                          const uint8_t channelId,
                                          const uint8_t type,
                                          const uint8_t scale,
                                          const float value,
                                          const bool isSetpointRequested ) override;

    void onUpdateFloatValue( const uint8_t nodeId,
                             const uint8_t command,
                             const uint8_t channelId,
                             const znet::meter::MeterType type,
                             const znet::meter::Scale scale,
                             const int32_t value,
                             const uint8_t precisionn ) override;

    void onUpdateSceneValue( const uint8_t nodeId,
                             const uint8_t ccId,
                             const uint8_t channelId,
                             const uint8_t sequence_number,
                             const uint8_t scene_number,
                             const std::string& key_atribute ) override;

    void onSceneActivated( const uint8_t nodeId,
                           const uint8_t command,
                           const uint8_t channelId,
                           const uint8_t sceneNumber ) override;

    void onUpdateRgbValue( const uint8_t nodeId,
                           const uint8_t ccId,
                           const uint8_t channelId,
                           ::znet::ColorComponentInfos&& components ) override;

    void onConfigurationReport( const uint8_t nodeId,
                                const uint8_t channelId,
                                const uint16_t configParamNum,
                                const uint8_t configParamSize,
                                const uint32_t configParamValue ) override;

    void onConfigurationRequestStatus( const uint8_t nodeId,
                                       const uint8_t configParamNum,
                                       znet::RequestStatus::Enum status ) override;

    void onNotification( const uint8_t nodeId,
                         const uint8_t channelId,
                         const uint8_t v1AlarmType,
                         const uint8_t v1AlarmLevel,
                         znet::NotificationType notification,
                         znet::NotificationEvent event,
                         const uint8_t* data,
                         const uint8_t dataSize ) override;

    void onUnknownNotification( const uint8_t nodeId,
                                const uint8_t channelId,
                                const uint8_t v1AlarmType,
                                const uint8_t v1AlarmLevel,
                                znet::NotificationType notification,
                                znet::NotificationEvent event,
                                const uint8_t* data,
                                const uint8_t dataSize ) override;

    void onAssociationSupportedGroupingResult( const uint8_t nodeId, const uint8_t count ) override;

    void onMultichannelAssociationResult( const uint8_t nodeId,
                                          const uint8_t channelId,
                                          const uint8_t groupId,
                                          const std::vector<std::pair<uint8_t, uint8_t>>& nodes ) override;

    void onAssociationResult( const uint8_t nodeId,
                              const uint8_t channelId,
                              const uint8_t groupId,
                              const std::vector<uint8_t>& nodes ) override;

    void onMasterCodeUpdated( const uint8_t nodeId, const uint8_t channelId, const std::string& masterCode ) override;

    void onUserCodesKeypadModeUpdated( const uint8_t nodeId,
                                       const uint8_t channelId,
                                       znet::UserCode::KeypadMode keypadMode ) override;

    void onUpdateScheduleEntryLockWeekDay( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_week_day_schedule_t& value ) override;

    void onUpdateScheduleEntryLockYearDay( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_year_day_schedule_t& value ) override;

    void onUpdateScheduleEntryLockDailyRepeating( znet_node_id_t nodeId,
                                                  znet_node_channel_id_t channelId,
                                                  const znet_daily_repeating_schedule_t& value ) override;

    void onHail( const uint8_t nodeId ) override;

    void onNif( const uint8_t nodeId ) override;

    void onSisUpdated( bool enabled ) override;

    void onWakeupIntervalUpdated( znet_node_id_t, std::chrono::seconds ) override;

    void fillManufacturerSpecific( znet_manufacturer_specific_report_t& data ) override;
    void fillVersion( znet_version_report_t& data ) override;
    void fillZWavePlusInfo( znet_zwaveplus_info_report_t& data ) override;
    void fillFirmwareLocale( std::string& locale ) override;

    void fillFirmwareUpdateInfo( znet_firmware_md_report_v1_t& dataV1,
                                 znet_firmware_md_report_v3_t& dataV3,
                                 znet_firmware_md_report_v5_t& dataV5 ) override;

    void onUpdateValueThermostatOperatingState( const uint8_t nodeId,
                                                const uint8_t channelId,
                                                const uint8_t command,
                                                znet::ThermostatOperatingState state ) override;

    void onUpdateThermostatSetbackValue( const uint8_t nodeId,
                                         const uint8_t channelId,
                                         const znet::thermostat_setback::OverrideType overrideType,
                                         const znet::thermostat_setback::Mode mode,
                                         const znet::thermostat_setback::SetbackValueOpt valueOpt ) override;

    void onUpdateSoundSwitchConfiguration( znet_node_id_t nodeId,
                                           znet_node_channel_id_t channelId,
                                           const znet_cmd_sound_switch_configuration_t& value ) override;

    void onUpdateSoundSwitchTonePlay( znet_node_id_t nodeId,
                                      znet_node_channel_id_t channelId,
                                      const znet_cmd_sound_switch_tone_play_t& value ) override;

    void onProvisioningListEntryAdded( const znet::ProvisioningListEntry& ) override;
    void onProvisioningListEntryUpdated( const znet::ProvisioningListEntry&,
                                         const znet::ProvisioningListEntry& ) override;
    void onProvisioningListEntryRemoved( uint32_t provisioningId ) override;

    bool isAllDevicesRemoved();

    void onUserCodeUpdated( znet_node_id_t nodeId,
                            znet_node_channel_id_t channelId,
                            ::znet::UserCode::UserCodeRanges&& userCodeRanges,
                            znet_supported_users_t usersCount,
                            ::znet::UserCode::UserPollStatus userPollingStatus ) override;

    znet::INodesProvider::NodeIdsSet getNodeIds() const override;

    std::unique_ptr<znet::INodeSetting> provideNodeSetting( uint16_t manufacturerId,
                                                            uint16_t productType,
                                                            uint16_t productId ) override;

    void onDeviceRediscoverProgressUpdate( znet_node_id_t nodeId,
                                           bool isOk,
                                           ::znet::ErrorOpt errorMessage,
                                           uint8_t progress ) override;

    void onNodeTargetFirmwareUpdateProgress( const znet_node_id_t nodeId,
                                             const uint8_t target,
                                             ::znet::FirmwareUpdate::UpdatingResult status,
                                             uint8_t progress ) override;

    void onIndicatorIdentify() override;

    void onUpdateBarrierSignalValue( znet_node_id_t nodeId,
                                     znet_node_channel_id_t channelId,
                                     uint8_t signalId,
                                     uint8_t value ) override;

    void onNodeCheckStatusUpdated( uint8_t nodeId, ::znet::ModeStatus status ) override;

    bool startChipBackup( bool backupCreation, std::function<void( bool )> resultCallback );

    void onAntiTheftStateUpdated( znet_node_id_t nodeId,
                                  znet_node_channel_id_t channelId,
                                  znet_command_class_t commandClassId,
                                  const ::znet::AntiTheftInfoOpt& antiTheftInfo ) override;

protected:
    ZNet(::znet::IProvisioningListProvider&, ::znet::IProvisioningListProvider& );

private:
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SendNif& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::HardReset& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StartInclude& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StopInclude& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StopSecurityInclude& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StartLearn& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StopLearn& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StartExclude& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StopExclude& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::StartRediscover& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::S2SelectKey& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::S2SetPublicKey& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetValue& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetValueMulticast& command ) override;
    void visit(::bus::BusAddr from, ::bus::BusAddr to, ::bus::command::SetThermostatMode& ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchLevelChange& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::ColorSwitchLevelChange& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RequestValue& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::RequestValueTermostatSetpoint& command ) override;
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::configuration::commands::AddConfiguration& command ) override;
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::configuration::commands::SetConfiguration& command ) override;
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::SetBarrierSignal& command ) override;
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::RequestBarrierSignal& command ) override;
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::configuration::commands::RequestConfiguration& command ) override;
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::configuration::commands::RemoveConfiguration& command ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::configuration::commands::SetDoorLockConfiguration& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::configuration::commands::RequestDoorLockConfiguration& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::configuration::commands::ResetAllConfigurationValues& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RequestDoorLockState& ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetUserCode& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RequestUserCode& command ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::UserCodeV2Operation& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::SetDailyRepeatingEntryLockSchedule& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::SetWeekDayEntryLockSchedule& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::SetYearDayEntryLockSchedule& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RemoveDailyRepeatingEntryLockSchedule& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RemoveWeekDayEntryLockSchedule& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RemoveYearDayEntryLockSchedule& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RequestDailyRepeatingEntryLockSchedule& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RequestWeekDayEntryLockSchedule& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RequestYearDayEntryLockSchedule& ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ReconstructNetwork& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::GetBackgrounRSSI& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::SoundSwitchConfigrationSet& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::SoundSwitchConfigrationGet& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::SoundSwitchTonePlaySet& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::SoundSwitchTonePlayGet& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::GpioButtonClicked& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningAdd& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningRemove& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningList& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningUpdate& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::ZwaveProvisioningGet& command ) override;

    void visit( bus::BusAddr, bus::BusAddr, ::bus::command::SendIdentify& ) override;

    void visit( bus::BusAddr, bus::BusAddr, ::bus::command::UpdatesNodesFirmware& ) override;
    void visit( bus::BusAddr, bus::BusAddr, ::bus::command::TimezoneWasUpdated& ) override;

    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::SetDevicePollIntervals& command ) override;

    void visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::CheckNodeValues& command ) override;

    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::CheckReachableStatus& command ) override;

    void visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::LockAntiTheft& command ) override;
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::LockAntiTheftDefault& command ) override;
    void visit( const ::bus::BusAddr from, const ::bus::BusAddr to, ::bus::command::UnlockAntiTheft& command ) override;
    void visit( const ::bus::BusAddr from,
                const ::bus::BusAddr to,
                ::bus::command::UnlockAntiTheftDefault& command ) override;

    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::RequestAlarmV1Value& ) override;

    bool startFirmwareUpdate( const common::DeviceFirmwareUpdateManager::Device& device,
                              const common::DeviceFirmwareUpdateManager::Target& target,
                              const common::DeviceFirmwareUpdateManager::Path& destination,
                              std::function<void( bool )> resultCallback ) override;

    void onFirmwareDownloadStatus( const common::DeviceFirmwareUpdateManager::ExtendedLink& link,
                                   bool isOk,
                                   uint8_t progress ) override;

    void addFirmwareUpdateTask( std::function<void()> task ) override;

    void onFirmwareUpdateStarted( const common::DeviceFirmwareUpdateManager::Device& device ) override;

    void onFirmwareUpdateFinished(
        const common::DeviceFirmwareUpdateManager::Device& device,
        const common::DeviceFirmwareUpdateManager::TargetsUpdateResults& targetsResults ) override;

    void notify(
        std::function<std::unique_ptr<::bus::command::SendGenericEventToLuaHandler>( std::string )> func ) override;

private:
    static uint8_t weekDaysFromArrayOfStrings( const ::common::ArrayValue& );
    static std::vector<uint8_t> parseDoorLockHandleIds( uint8_t );

    static std::shared_ptr<::gateways::zwave::devices::Device> getDeviceAndSetReachable( uint8_t nodeId );
    static znet::ZClass* getCommandClass( uint8_t nodeId, uint8_t channelId, uint8_t commandClassId );

    static void updateNodeTargetFirmwareTask( const common::DeviceFirmwareUpdateManager::Device& device,
                                              const common::DeviceFirmwareUpdateManager::Target& target,
                                              const common::DeviceFirmwareUpdateManager::Path& destination );

    static ::znet::ColorComponentID rgbValueColorToZnetColor(::common::RgbValue::Color );

    static void chipBackupTask( bool backupCreation, std::function<void( bool )> resultCallback );

private:
    std::atomic_bool m_isZnetInitCompleted{ false };
    bool m_allDevicesRemoved;
    std::string m_deviceSettingsFilePath;
    std::unique_ptr<common::DeviceFirmwareUpdateManager> m_deviceFirmwareUpdateManager;
    ITimerProvider::TimerUPtr m_pollTimer;
};

}  // namespace zwave
}  // namespace gateways
