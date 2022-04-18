#pragma once

#include <stdint.h>
#include <string>
#include <list>
#include <vector>
#include <chrono>
#include <mutex>

#include <Bus.h>
#include "lua/responce/AddDevice.h"
#include "lua/responce/RemoveDevice.h"
#include <IValue.h>

namespace common {
class JsonStreamGenerator;
};  // namespace common

namespace storage {
class File;
}  // namespace storage

namespace core {

class HouseModesManager : public bus::IBusClient, public common::IInitialize {
public:
    enum class Result {
        OK = 0,

        WrongArguments,
        Busy,
        NotFound,
        NotSupported,
        MaxNumberPresetsReached,
        SystemError
    };
    using PresetId = uint8_t;

    constexpr static PresetId PRESET_ID_CURRENT = 0;
    constexpr static PresetId PRESET_ID_INVALID = UINT8_MAX;
    constexpr static PresetId PRESET_ID_MIN = 1;
    constexpr static PresetId PRESET_ID_MAX = PRESET_ID_INVALID - 1;

    struct SensorState {
        std::string id;
        bool armed;
    };

    struct PresetInfo {
        std::string id;
        std::string name;
        bool defaultState;
        std::vector<std::string> disarmed_devices;
        std::vector<std::string> alarms_off_devices;
    };

public:
    static HouseModesManager& get();

    void update();

    bool isDeviceModed( const std::string& deviceId, const char* deviceCat, const char* deviceSubCat ) const;
    bool isDeviceModed( const std::string& deviceId ) const;
    bool isDeviceArmed( const std::string& deviceId, const char* deviceCat, const char* deviceSubCat ) const;
    bool isDeviceArmed( const std::string& deviceId ) const;
    bool isAlarmDevice( const char* deviceCat, const char* deviceSubCat ) const;
    bool isAlarmDeviceActivated( const char* deviceId ) const;

    std::vector<SensorState> getSensorStates() const;
    Result sendArmedStateChangedBroadcasts( const std::vector<SensorState>& oldStates,
                                            const std::vector<SensorState>& newStates ) const;

    /// INFO: API -> http://log.ezlo.lan/new/hub/hub_platform/house_modes_manager/
    Result getPresets( common::JsonStreamGenerator& result ) const;
    Result getCurrent( common::JsonStreamGenerator& result ) const;
    Result switchPreset( common::JsonStreamGenerator& result, const PresetId presetId );
    Result switchPreset( common::JsonStreamGenerator& result, const std::string& name );
    bool switchPreset( const PresetId presetId );
    bool switchPreset( const std::string& presetId );
    Result cancelSwitchPreset( common::JsonStreamGenerator& result );
    Result setSwitchToDelay( common::JsonStreamGenerator& result, uint32_t delay );
    Result setAlarmDelay( common::JsonStreamGenerator& result, uint32_t delay );
    Result setDefaultNotifications( PresetId presetId );
    Result setNotifications( common::JsonStreamGenerator& result,
                             PresetId presetId,
                             bool all,
                             std::vector<std::string>&& users );
    Result setNotificationAll( common::JsonStreamGenerator& result, PresetId presetId, bool all );
    Result addNotification( common::JsonStreamGenerator& result, PresetId presetId, const char* userId );
    Result removeNotification( common::JsonStreamGenerator& result, PresetId presetId, const char* userId );
    Result getCurrentNotifications( common::JsonStreamGenerator& result ) const;
    Result setDisarmedDefault( common::JsonStreamGenerator& result, PresetId presetId, bool def );
    Result addDisarmedDevice( common::JsonStreamGenerator& result, PresetId presetId, const char* deviceId );
    Result removeDisarmedDevice( common::JsonStreamGenerator& result, PresetId presetId, const char* deviceId );
    Result setAlarmsOffDefault( common::JsonStreamGenerator& result, PresetId presetId, bool def );
    Result addAlarmsOff( common::JsonStreamGenerator& result, PresetId presetId, const char* deviceId );
    Result removeAlarmsOff( common::JsonStreamGenerator& result, PresetId presetId, const char* deviceId );
    Result setDeviceArmed( const char* deviceId, bool armed = true, bool* prev_state = nullptr );
    bool isNotificationSendToUser( const gateways::ItemInfo& itemInfo, const common::IValue& value );
    uint8_t getCurrentPresetId() const;
    std::string currentPreset() const;
    std::string cancelSwitchPreset();
    void getPresetsIds( std::vector<std::string>& result ) const;
    PresetInfo getPresetInfo( std::string presetId ) const;
    PresetInfo findPresetByName( std::string name );
    std::string parentDeviceName( const gateways::ItemInfo& itemInfo, const common::IValue& value );

private:
    bool initializeImpl() override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddDevice& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::RemoveDevice& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateItem& command ) override;

private:
    constexpr static char PresetsFile[] = "/vol/house.mod";
    constexpr static uint16_t StorageVersion = 6;

    Result save() const;
    Result load();

    void turnAllAlarmsOn() const;
    bool sendNotifyAllBroadcast( PresetId presetId, bool all ) const;
    bool sendNotificationRemoveBroadcast( PresetId presetId, const std::string& id ) const;
    bool sendNotificationAddBroadcast( PresetId presetId, const std::string& id ) const;
    bool sendModesChangedBroadcast( PresetId presetId,
                                    const std::string& name,
                                    std::unique_ptr<common::IValue> value ) const;

    bool needPowerStateSensorItemNotification( const gateways::ItemInfo& itemInfo, const common::IValue& value );

private:
    struct Preset {
        PresetId id = PRESET_ID_INVALID;
        std::string name;
        std::string description;
        uint32_t switchToDelay = 0;  // sec
        uint32_t alarmDelay = 0;
        bool notifyAll = false;
        std::list<std::string> notifications;
        // std::list< deviceType > armedTypeDevices;
        bool disarmedDefault = true;
        std::list<SensorState> sensorStates;
        bool alarmsOffDefault = true;
        std::list<std::string /*deviceId*/> alarmsOff;
    };
    struct AlarmTimePoint {
        std::string deviceId;
        std::chrono::time_point<std::chrono::steady_clock> timePoint;
    };
    std::list<Preset> m_presets;
    std::list<SensorState> m_sensorStates;
    PresetId m_currentPreset = PRESET_ID_INVALID;
    PresetId m_newPreset = PRESET_ID_INVALID;
    std::chrono::time_point<std::chrono::steady_clock> m_switchTimePoint;
    std::mutex m_mtxAlarms;
    std::list<AlarmTimePoint> m_alarms;
};

}  // namespace core
