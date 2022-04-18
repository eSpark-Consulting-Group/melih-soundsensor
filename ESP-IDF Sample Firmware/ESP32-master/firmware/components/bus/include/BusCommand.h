#pragma once

#include "BusAddr.h"
#include "sdkconfig.h"

#include <list>
#include <atomic>
#include <future>

namespace cloud {

namespace responses {
class HandleEventCloudResponseCommand;
class HandleEventCloudDeviceAdded;
class HandleEventCloudDeviceRemoved;
class HandleEventCloudDeviceUpdated;
}  // namespace responses

}  // namespace cloud

namespace cloud_client {
namespace commands {
static constexpr auto BUS_NAME = "cloud_client_cmd";
class HubProvisionChanged;
class FirmwareUpdateCompleted;
class ProvisionSyncingCompleted;
}  // namespace commands
namespace responces {
static constexpr auto BUS_NAME = "cloud_client_resp";
class ConnectedToCloud;
class DisconnectedFromCloud;
class ReceivedPeersProvision;
}  // namespace responces
}  // namespace cloud_client

namespace scenes {

namespace commands {
static constexpr auto LOCAL_BUS_NAME = "local_scenes_cmd";
static constexpr auto GLOBAL_BUS_NAME = "global_scenes_cmd";
class SetSceneName;
class SetSceneHouseModes;
class CreateScene;
class EditScene;
class DeleteScene;
class ScenesList;
class ScenesData;
class TimeList;
class SceneEnabledSet;
class RunScene;
class JsonifyScene;
class EnumerateLocalScenes;
}  // namespace commands

namespace responces {
class SceneAdded;
class GlobalSceneAdded;
class SceneDeleted;
class GlobalSceneDeleted;
class SceneChanged;
class GlobalSceneChanged;
class SceneRunProgress;
class GlobalSceneRunProgress;
}  // namespace responces

}  // namespace scenes

namespace settings {

namespace commands {
class DeviceSettingDictionaryValueSet;
class GatewaySettingValueSet;
class DeviceSettingValueSet;
class DeviceSettingReset;
}  // namespace commands

namespace responces {
class SettingAdded;
class SettingRemoved;
class SettingValueSetted;
class SettingUpdated;
class GatewaySettingValueSetted;
class SettingStatusUpdated;
class GatewaySettingStatusUpdated;
class GatewaySettingAdded;
class GatewaySettingRemoved;
class SettingDictionaryValueUpdated;

class SettingDictionaryStatusUpdated;
}  // namespace responces

}  // namespace settings

namespace gateways::commands {
class ZwaveProvisioningAdd;
class ZwaveProvisioningRemove;
class ZwaveProvisioningList;
class ZwaveProvisioningUpdate;
class ZwaveProvisioningGet;
class ProvisioningListEntryAdded;
class ProvisioningListEntryUpdated;
class ProvisioningListEntryRemoved;
}  // namespace gateways::commands

namespace nma {
namespace responces {
class HubSoftwareUpdateProgress;
}  // namespace responces
}  // namespace nma

namespace core {
namespace commands {
static constexpr auto BUS_NAME = "core_cmd";
class Shutdown;
}  // namespace commands
}  // namespace core

namespace configuration {
namespace commands {
class AddConfiguration;
class SetConfiguration;
class RequestConfiguration;
class RemoveConfiguration;
class SetDoorLockConfiguration;
class RequestDoorLockConfiguration;
class ResetAllConfigurationValues;
}  // namespace commands
}  // namespace configuration

namespace network {
namespace wifi_srv2 {
namespace command {
constexpr auto BUS_NAME = "WiFiGenericClass_commands";
class JoinToLastAccessPoint;
class JoinToAccessPoint;
class DisjoinFromAccessPoint;
class StartAccessPoint;
class StopAccessPoint;
class StartWifiScan;
class StopWifiScan;
}  // namespace command
namespace responce {
static constexpr auto BUS_NAME = "WiFiGenericClass_responces";
class ConnectedToWifiStation;
class DisconnectedFromWifiStation;
class DiscoveredWifiAccessPoints;
class DiscoveredWifiBeacons;
}  // namespace responce
}  // namespace wifi_srv2
}  // namespace network

namespace time_service {
namespace command {
static constexpr auto BUS_NAME = "time_service_cmd";
class SetTimezone;
}  // namespace command
namespace responce {
static constexpr auto BUS_NAME = "time_service_resp";
class TimeEstablished;
}  // namespace responce
}  // namespace time_service

namespace lua {
namespace responce {
static constexpr auto BUS_NAME = "lua_responce";
struct HubSendBroadcast;
struct AddItem;
struct UpdateItem;
struct AddDevice;
struct RemoveDevice;
struct UpdateDevice;
struct UpdateItemDictionary;
}  // namespace responce
}  // namespace lua

namespace dev_dis {
namespace responce {
static constexpr auto BUS_NAME = "device_discovery_resp";
struct DiscoveredDevices;
struct DeviceAdded;
struct DeviceRemoved;
struct DeviceEdited;
}  // namespace responce
}  // namespace dev_dis

namespace bus {
namespace command {

static constexpr auto GPIO_BUTTON_BUS_CLIENT_NAME = "gpio_button";

namespace nma {
struct RequestTimeZones;
struct ServerRequest;
struct ClientClbk;
struct CloudControllerAbstractSetResponce;
}  // namespace nma

struct CloudForceRemove;
struct CloudItemValueSet;
class CloudDictionaryValue;
struct FavoriteUpdated;
class HandleEventBase;
class HandleEvent;
class HandleEventUpdateProductName;
struct HardReset;
struct ModesAlarmsOffAdded;
struct ModesAlarmsOffDefault;
struct ModesAlarmsOffRemoved;
struct ModesChanged;
struct ModesDisarmedDevicesAdded;
struct ModesDisarmedDevicesDefault;
struct ModesDisarmedDevicesRemoved;
struct ModesNotificationsAdded;
struct ModesNotificationsNotifyAll;
struct ModesNotificationsRemoved;
struct ModesSwitched;
struct PluginRun;
struct RemoteLog;
struct RemoveItem;
struct RequestAllValue;
struct RequestValue;
struct RequestValueTermostatSetpoint;
struct RequestUserCode;
class RequestDoorLockState;
struct ReconstructNetwork;
struct GetBackgrounRSSI;
struct RoomsCreated;
struct RoomsDeleted;
struct RoomsReordered;
struct S2SelectKey;
struct S2SetPublicKey;
#if CONFIG_REMOTE_LOGGING
#ifdef IS_SAVE_CRASH_DUMP
struct SendCrashLog;
#endif  // IS_SAVE_CRASH_DUMP
#endif  // CONFIG_REMOTE_LOGGING
struct SendNif;
class SetBarrierSignal;
struct SetHandler;
struct SetValue;
struct SetValueMulticast;
class SetThermostatMode;
struct SetUserCode;
struct StartExclude;
class StartInclude;
struct StartLearn;
class StartRediscover;
struct StopExclude;
struct StopInclude;
struct StopSecurityInclude;
struct StopLearn;
struct TimezoneWasUpdated;
class UpdateGateway;
class SwitchValueUpdated;
class SwitchValueSet;
class SetConsumedEnergyMeter;
class ElectricMeterValuesUpdated;
class ResetEnergyMeter;
class WaterLeakValueUpdate;
class SoundSwitchConfigrationGet;
class SoundSwitchTonePlayGet;
class SoundSwitchConfigrationSet;
class SoundSwitchTonePlaySet;
class GpioButtonClicked;
class GpioButtonDoubleClicked;
class GpioButtonLongClicked;
class GpioButtonVeryLongClicked;
class GpioButtonVeryVeryLongClicked;
class LocalModeFeatureStatusUpdate;
class CloudDevicesFeatureStatusUpdate;
class AdvancedScenesFeatureStatusUpdate;
class SetDailyRepeatingEntryLockSchedule;
class SetWeekDayEntryLockSchedule;
class SetYearDayEntryLockSchedule;
class RequestDailyRepeatingEntryLockSchedule;
class RequestWeekDayEntryLockSchedule;
class RequestYearDayEntryLockSchedule;
class RequestBarrierSignal;
class RemoveDailyRepeatingEntryLockSchedule;
class RemoveWeekDayEntryLockSchedule;
class RemoveYearDayEntryLockSchedule;
class SendGenericEventToLuaHandler;
class SendIdentify;
class UpdatesNodesFirmware;
class HubDeviceFirmwareUpdate;
struct HubSettingsApply;
class SetDevicePollIntervals;
class CheckNodeValues;
class CheckReachableStatus;
class SwitchLevelChange;
class ColorSwitchLevelChange;
class UserCodeV2Operation;
class RestoreChipBackup;
class CreateChipBackup;
class LockAntiTheft;
class LockAntiTheftDefault;
class UnlockAntiTheft;
class UnlockAntiTheftDefault;
class RequestAlarmV1Value;

}  // namespace command

namespace lua_event {
static constexpr auto BUS_NAME = "lua_cmd";
class GenericLuaEvent;
}  // namespace lua_event

struct IBusCommandVisitor {
    virtual ~IBusCommandVisitor() = default;

    // nma
    virtual void visit( const BusAddr from, const BusAddr to, command::nma::RequestTimeZones& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::nma::ServerRequest& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::nma::ClientClbk& command ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        command::nma::CloudControllerAbstractSetResponce& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, nma::responces::HubSoftwareUpdateProgress& command ) {}

    // example !!!
    virtual void visit( const BusAddr from, const BusAddr to, command::CloudForceRemove& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::CloudItemValueSet& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::CloudDictionaryValue& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::PluginRun& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SetValue& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SetThermostatMode& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SetValueMulticast& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SwitchLevelChange& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ColorSwitchLevelChange& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RequestAllValue& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RequestValue& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RequestValueTermostatSetpoint& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RequestUserCode& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RequestDoorLockState& ) {}
    virtual void visit( BusAddr from, BusAddr to, ::bus::command::RequestAlarmV1Value& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ReconstructNetwork& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::GetBackgrounRSSI& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::UpdateGateway& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RemoteLog& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RemoveItem& command ) {}
#if CONFIG_REMOTE_LOGGING
#ifdef IS_SAVE_CRASH_DUMP
    virtual void visit( const BusAddr from, const BusAddr to, command::SendCrashLog& command ) {}
#endif  // IS_SAVE_CRASH_DUMP
#endif  // CONFIG_REMOTE_LOGGING
    virtual void visit( const BusAddr from, const BusAddr to, command::SendNif& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::StartInclude& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::StopInclude& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::StopSecurityInclude& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::StartLearn& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::StartRediscover& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::StopLearn& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::StartExclude& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::StopExclude& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::HardReset& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::S2SelectKey& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::S2SetPublicKey& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SetHandler& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SetUserCode& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::SetDailyRepeatingEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::SetWeekDayEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::SetYearDayEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::RequestDailyRepeatingEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::RequestWeekDayEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::RequestYearDayEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::RemoveDailyRepeatingEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::RemoveWeekDayEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::RemoveYearDayEntryLockSchedule& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::HandleEventUpdateProductName& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesSwitched& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesNotificationsNotifyAll& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesNotificationsAdded& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesNotificationsRemoved& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesAlarmsOffDefault& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesAlarmsOffAdded& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesAlarmsOffRemoved& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesChanged& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesDisarmedDevicesDefault& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesDisarmedDevicesAdded& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ModesDisarmedDevicesRemoved& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RoomsCreated& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RoomsDeleted& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::RoomsReordered& command ) {}

    // Scenes commands
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::SetSceneName& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::SetSceneHouseModes& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::CreateScene& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::EditScene& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::DeleteScene& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::ScenesList& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::ScenesData& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::TimeList& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::SceneEnabledSet& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::RunScene& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::JsonifyScene& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, scenes::commands::EnumerateLocalScenes& command ) {}
    // Scenes responces
    virtual void visit( const BusAddr from, const BusAddr to, const scenes::responces::SceneAdded& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const scenes::responces::GlobalSceneAdded& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const scenes::responces::SceneDeleted& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const scenes::responces::GlobalSceneDeleted& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const scenes::responces::SceneRunProgress& command ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const scenes::responces::GlobalSceneRunProgress& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const scenes::responces::SceneChanged& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const scenes::responces::GlobalSceneChanged& command ) {}

    virtual void visit( const BusAddr from, const BusAddr to, command::FavoriteUpdated& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SwitchValueUpdated& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SwitchValueSet& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SetConsumedEnergyMeter& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ElectricMeterValuesUpdated& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ResetEnergyMeter& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::WaterLeakValueUpdate& command ) {}

    virtual void visit( const BusAddr from, const BusAddr to, command::SoundSwitchConfigrationSet& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SoundSwitchConfigrationGet& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SoundSwitchTonePlaySet& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::SoundSwitchTonePlayGet& command ) {}

    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        cloud::responses::HandleEventCloudResponseCommand& command ) {}

    virtual void visit( const BusAddr from, const BusAddr to, cloud::responses::HandleEventCloudDeviceAdded& command ) {
    }

    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        cloud::responses::HandleEventCloudDeviceRemoved& command ) {}

    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        cloud::responses::HandleEventCloudDeviceUpdated& command ) {}

    virtual void visit( const BusAddr from, const BusAddr to, command::GpioButtonClicked& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::GpioButtonDoubleClicked& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::GpioButtonLongClicked& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::GpioButtonVeryLongClicked& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::GpioButtonVeryVeryLongClicked& command ) {}

    virtual void visit( const BusAddr from, const BusAddr to, command::LocalModeFeatureStatusUpdate& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::CloudDevicesFeatureStatusUpdate& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::AdvancedScenesFeatureStatusUpdate& command ) {}

    // settings
    virtual void visit( const BusAddr from, const BusAddr to, settings::commands::DeviceSettingValueSet& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, settings::commands::GatewaySettingValueSet& command ) {}

    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        settings::commands::DeviceSettingDictionaryValueSet& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, settings::commands::DeviceSettingReset& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, settings::responces::SettingAdded& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, settings::responces::SettingRemoved& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, settings::responces::SettingValueSetted& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, settings::responces::SettingUpdated& command ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        settings::responces::GatewaySettingValueSetted& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, settings::responces::SettingStatusUpdated& command ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,

                        settings::responces::GatewaySettingStatusUpdated& command ) {}

    virtual void visit( const BusAddr from, const BusAddr to, settings::responces::GatewaySettingAdded& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, settings::responces::GatewaySettingRemoved& command ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        settings::responces::SettingDictionaryValueUpdated& command ) {}

    virtual void visit( BusAddr, BusAddr, ::settings::responces::SettingDictionaryStatusUpdated& ) {}

    // gateways
    virtual void visit( const BusAddr from, const BusAddr to, gateways::commands::ZwaveProvisioningAdd& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, gateways::commands::ZwaveProvisioningRemove& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, gateways::commands::ZwaveProvisioningList& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, gateways::commands::ZwaveProvisioningUpdate& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, gateways::commands::ZwaveProvisioningGet& command ) {}

    virtual void visit( const bus::BusAddr from, const bus::BusAddr to, command::RestoreChipBackup& command ) {}
    virtual void visit( const bus::BusAddr from, const bus::BusAddr to, command::CreateChipBackup& command ) {}

    // ezlo_sys
    virtual void visit( const BusAddr from, const BusAddr to, const core::commands::Shutdown& command ) {}

    // cloud_client
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const cloud_client::responces::ConnectedToCloud& command ) {}

    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const cloud_client::responces::DisconnectedFromCloud& command ) {}

    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        cloud_client::responces::ReceivedPeersProvision& command ) {}

    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const cloud_client::commands::HubProvisionChanged& command ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const cloud_client::commands::FirmwareUpdateCompleted& command ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const cloud_client::commands::ProvisionSyncingCompleted& command ) {}

    // configuration
    virtual void visit( const BusAddr from, const BusAddr to, configuration::commands::AddConfiguration& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, configuration::commands::SetConfiguration& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, configuration::commands::RequestConfiguration& command ) {
    }
    virtual void visit( const BusAddr from, const BusAddr to, configuration::commands::RemoveConfiguration& command ) {}
    virtual void visit( BusAddr, BusAddr, ::configuration::commands::SetDoorLockConfiguration& ) {}
    virtual void visit( BusAddr, BusAddr, ::configuration::commands::RequestDoorLockConfiguration& ) {}
    virtual void visit( BusAddr, BusAddr, ::configuration::commands::ResetAllConfigurationValues& ) {}

    // Wifi service 2
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::command::JoinToLastAccessPoint& cmd ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::command::JoinToAccessPoint& cmd ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::command::DisjoinFromAccessPoint& cmd ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::command::StartAccessPoint& cmd ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::command::StopAccessPoint& cmd ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const network::wifi_srv2::command::StartWifiScan& cmd ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const network::wifi_srv2::command::StopWifiScan& cmd ) {}

    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::responce::ConnectedToWifiStation& cmd ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::responce::DisconnectedFromWifiStation& cmd ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::responce::DiscoveredWifiAccessPoints& cmd ) {}
    virtual void visit( const BusAddr from,
                        const BusAddr to,
                        const network::wifi_srv2::responce::DiscoveredWifiBeacons& cmd ) {}

    virtual void visit( const BusAddr from, const BusAddr to, const dev_dis::responce::DiscoveredDevices& cmd ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const dev_dis::responce::DeviceAdded& cmd ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const dev_dis::responce::DeviceRemoved& cmd ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const dev_dis::responce::DeviceEdited& cmd ) {}

    // Gateways
    virtual void visit( BusAddr, BusAddr, const ::gateways::commands::ProvisioningListEntryAdded& ) {}
    virtual void visit( BusAddr, BusAddr, const ::gateways::commands::ProvisioningListEntryUpdated& ) {}
    virtual void visit( BusAddr, BusAddr, const ::gateways::commands::ProvisioningListEntryRemoved& ) {}

    virtual void visit( BusAddr, BusAddr, ::bus::lua_event::GenericLuaEvent& ) {}
    virtual void visit( BusAddr, BusAddr, ::bus::command::SendGenericEventToLuaHandler& ) {}
    virtual void visit( BusAddr, BusAddr, ::bus::command::SendIdentify& ) {}

    virtual void visit( BusAddr, BusAddr, ::bus::command::HubDeviceFirmwareUpdate& ) {}
    virtual void visit( BusAddr, BusAddr, ::bus::command::UpdatesNodesFirmware& ) {}
    virtual void visit( BusAddr, BusAddr, ::bus::command::UserCodeV2Operation& ) {}

    virtual void visit( BusAddr, BusAddr, ::bus::command::TimezoneWasUpdated& ) {}

    virtual void visit( const BusAddr from, const BusAddr to, bus::command::SetBarrierSignal& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::RequestBarrierSignal& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::HubSettingsApply& command ) {}

    virtual void visit( const BusAddr from, const BusAddr to, bus::command::SetDevicePollIntervals& command ) {}

    virtual void visit( const BusAddr, const BusAddr, ::bus::command::CheckNodeValues& ) {}
    virtual void visit( const BusAddr, const BusAddr, ::bus::command::CheckReachableStatus& ) {}

    // time_service
    virtual void visit( const BusAddr, const BusAddr, time_service::command::SetTimezone& ) {}
    virtual void visit( const BusAddr from, const BusAddr to, const time_service::responce::TimeEstablished& command ) {
    }

    // lua responce
    virtual void visit( const BusAddr from, const BusAddr to, lua::responce::HubSendBroadcast& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, lua::responce::AddDevice& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, lua::responce::RemoveDevice& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, lua::responce::UpdateDevice& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, lua::responce::AddItem& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, lua::responce::UpdateItem& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, lua::responce::UpdateItemDictionary& command ) {}

    virtual void visit( const BusAddr from, const BusAddr to, bus::command::LockAntiTheft& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::LockAntiTheftDefault& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::UnlockAntiTheft& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, bus::command::UnlockAntiTheftDefault& command ) {}
};

class IBusCommand {
    friend class Bus;
    friend class IBusClient;
    friend bool sendFromNowhere( const char* to, std::unique_ptr<IBusCommand>&& msg );
    friend bool sendFromNowhere( const BusAddr to, std::unique_ptr<IBusCommand>&& msg );

    std::atomic<int> m_refCtr = 1;
    BusAddr from = BusAddrInvalid, to = BusAddrInvalid;
    bool isGroup = false;

    inline void unref() {
        if( 0 == ( --m_refCtr ) ) {
            delete this;
        }
    }

    inline void setFromTo( const BusAddr _from, const BusAddr _to ) {
        from = _from;
        to = _to;
    }

public:
    virtual ~IBusCommand() = default;

    virtual void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) = 0;
};

using BusMsg = IBusCommand;

template <typename ResponceType>
class BusCommandWithDefaultInitializedResponse : public IBusCommand {
public:
    ~BusCommandWithDefaultInitializedResponse() {
        if( !isResponceWasSet() ) {
            setResponce( {} );
        }
    }

    inline auto getFuture() {
        return m_promise.get_future();
    }

    inline void setResponce( ResponceType&& responce ) {
        m_promise.set_value( std::move( responce ) );
        m_isResponceWasSet = true;
    }

    template <typename... Args>
    inline void setResponce( Args&&... args ) {
        setResponce( ResponceType( std::forward<Args>( args )... ) );
        m_isResponceWasSet = true;
    }

    inline bool isResponceWasSet() const {
        return m_isResponceWasSet;
    }

private:
    std::promise<ResponceType> m_promise;
    bool m_isResponceWasSet = false;
};

template <typename ResponseType, const ResponseType& responceWasntSet>
class BusCommandResp : public BusCommandWithDefaultInitializedResponse<ResponseType> {
public:
    ~BusCommandResp() {
        if( !BusCommandWithDefaultInitializedResponse<ResponseType>::isResponceWasSet() ) {
            BusCommandWithDefaultInitializedResponse<ResponseType>::setResponce( responceWasntSet );
        }
    }
};

}  // namespace bus
