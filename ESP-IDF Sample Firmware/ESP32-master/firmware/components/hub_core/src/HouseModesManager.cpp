#include <HouseModesManager.h>

#include <JsonStreamGenerator.h>
#include <NmaBufferStream.h>
#include <CommonResources.h>

#include <StorageArchiveFileFactory.h>
#include <StorageFile.h>
#include <Storage.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <DeviceInfo.h>
#include <ObjectSerializer.h>
#include <values/BoolValue.h>

#include <BusCommandModesDisarmedDevicesDefault.h>
#include <BusCommandModesDisarmedDevicesAdded.h>
#include <BusCommandModesDisarmedDevicesRemoved.h>
#include <BusCommandModesAlarmsOffDefault.h>
#include <BusCommandModesAlarmsOffAdded.h>
#include <BusCommandModesAlarmsOffRemoved.h>
#include <BusCommandModesChanged.h>
#include <BusCommandModesNotificationsNotifyAll.h>
#include <BusCommandModesNotificationsAdded.h>
#include <BusCommandModesNotificationsRemoved.h>
#include <BusCommandModesSwitched.h>
#include "lua/responce/UpdateDevice.h"
#include "lua/responce/UpdateItem.h"
#include <BusCommandCloudItemValueSet.h>

#include <DebugUtils.h>
#include <Utils.h>

#include <set>
#include <algorithm>
#include <type_traits>
#include <tuple>
#include <utility>
#include <string.h>
#include <inttypes.h>
#include <cstdlib>

#include "values/IntValue.h"
#include "values/StringValue.h"
#include "values/BoolValue.h"

#include <IValueCast.h>
#include <GatewayUtils.h>

namespace core {

namespace {

using namespace storage;
using BlobType = storage::KeyValueStorage::BlobType;

using namespace common::resources;

struct CatSubCat {
    const char* cat;
    const char* sub_cat;
};

// Sensors
const struct CatSubCat HomeCatSubCat[] = { { "security_sensor", "smoke" },
                                           { "security_sensor", "co" },
                                           { "security_sensor", "glass" },
                                           { "security_sensor", "leak" },
                                           { "security_sensor", "heat" } };

const struct CatSubCat DefaultCatSubCat[] = { { "security_sensor", "smoke" }, { "security_sensor", "co" },
                                              { "security_sensor", "glass" }, { "security_sensor", "motion" },
                                              { "security_sensor", "door" },  { "security_sensor", "leak" },
                                              { "security_sensor", "heat" } };

// Alarms
const struct CatSubCat HomeAlarmsCatSubCat[] = { { "siren", nullptr } };

const struct CatSubCat DefaultAlarmsCatSubCat[] = { { "siren", nullptr }, { "camera", nullptr } };

const char* PreferredAlarmItemTypes[] = { "siren", "switch", "basic" };

/// INFO: Away, Night, Vacation as Default

namespace {
constexpr char HOME_MODE[] = "Home";
constexpr char AWAY_MODE[] = "Away";
constexpr char NIGHT_MODE[] = "Night";
constexpr char VACATION_MODE[] = "Vacation";

constexpr auto HOME_MODE_ID = 1;
constexpr auto AWAY_MODE_ID = 2;
constexpr auto NIGHT_MODE_ID = 3;
constexpr auto VACATION_MODE_ID = 4;

// Modes delay values: [switch_to_delay, alarm_delay]
constexpr int DEFAULT_HOME_MODE_DELAY[2] = { 0, 0 };
constexpr int DEFAULT_AWAY_MODE_DELAY[2] = { 30, 30 };
constexpr int DEFAULT_NIGHT_MODE_DELAY[2] = { 30, 30 };
constexpr int DEFAULT_VACATION_MODE_DELAY[2] = { 30, 30 };

}  // namespace

struct DefaultPreset {
    HouseModesManager::PresetId id;
    const char* name;
    const char* description;
    uint32_t switch_to_delay;  // sec
    uint32_t alarm_delay;      // sec
    bool notify_all;
    // list of notifications
    // list of device categories and subcategories
} defaultPresets[] = {
    { HOME_MODE_ID, HOME_MODE, "", DEFAULT_HOME_MODE_DELAY[0], DEFAULT_HOME_MODE_DELAY[1], true },
    { AWAY_MODE_ID, AWAY_MODE, "", DEFAULT_AWAY_MODE_DELAY[0], DEFAULT_AWAY_MODE_DELAY[1], true },
    { NIGHT_MODE_ID, NIGHT_MODE, "", DEFAULT_NIGHT_MODE_DELAY[0], DEFAULT_NIGHT_MODE_DELAY[1], true },
    { VACATION_MODE_ID, VACATION_MODE, "", DEFAULT_VACATION_MODE_DELAY[0], DEFAULT_VACATION_MODE_DELAY[1], true }
};
const size_t defaultPresetsCount = sizeof( defaultPresets ) / sizeof( defaultPresets[0] );
const uint16_t defaultVersion = 1;

template <class T, class R>
HouseModesManager::Result forDevices( T filter, R proc ) {
    auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !devices ) {
        return HouseModesManager::Result::SystemError;
    }
    auto it = devices->cbegin(), end = devices->cend();
    for( ; it != end; ++it ) {
        auto val = it.value();

        if( !val.is_type<BlobType>() ) {
            FUNC_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
            continue;
        }
        gateways::BaseDeviceInfo deviceInfo;
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( *it, std::move( val.get<BlobType>() ), deviceInfo ) ) {
            FUNC_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );

            continue;
        }
        if( filter( deviceInfo ) ) {
            proc( deviceInfo );
        }
    }
    return HouseModesManager::Result::OK;
}

}  // namespace

constexpr HouseModesManager::PresetId HouseModesManager::PRESET_ID_CURRENT;
constexpr HouseModesManager::PresetId HouseModesManager::PRESET_ID_MIN;
constexpr HouseModesManager::PresetId HouseModesManager::PRESET_ID_MAX;

constexpr char HouseModesManager::PresetsFile[];
constexpr uint16_t HouseModesManager::StorageVersion;

HouseModesManager& HouseModesManager::get() {
    static HouseModesManager houseModesManager;
    return houseModesManager;
}

bool HouseModesManager::initializeImpl() {
    if( !bus::Bus::get().join( lua::responce::BUS_NAME, *this ) ) {
        FUNC_LOGE( "house mode bus's client has not been joined" );
        return false;
    }

    m_switchTimePoint = std::chrono::steady_clock::now();
    return load() == HouseModesManager::Result::OK;
}

void HouseModesManager::update() {
    tryReceive( 0 );

    if( m_newPreset != PRESET_ID_INVALID ) {
        auto now = std::chrono::steady_clock::now();
        if( now >= m_switchTimePoint ) {
            if( m_newPreset != PRESET_ID_CURRENT ) {
                const auto oldSensorStates = getSensorStates();
                auto cmd = std::make_unique<bus::command::ModesSwitched>();
                if( cmd ) {
                    char modeId[32];
                    if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_currentPreset ) > 0 ) {
                        cmd->m_from = modeId;
                    }
                    if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_newPreset ) > 0 ) {
                        cmd->m_to = modeId;
                    }
                    cmd->m_status = "done";
                    cmd->m_switchDelay = 0;
                }
                m_currentPreset = m_newPreset;
                if( cmd ) {
                    send( "nma", std::move( cmd ) );
                }
                m_sensorStates.clear();
                if( Result::OK != save() ) {
                    FUNC_LOGE( "Failed to save new house mode" );
                }
                sendArmedStateChangedBroadcasts( oldSensorStates, getSensorStates() );
            }
            m_newPreset = PRESET_ID_INVALID;
        }
    }
    bool turnOnAlarms{ false };
    {
        std::lock_guard<std::mutex> lg( m_mtxAlarms );
        if( !m_alarms.empty() ) {
            auto now = std::chrono::steady_clock::now();
            auto alarm = m_alarms.begin();
            while( alarm != m_alarms.end() ) {
                bool erase{ false };
                if( now >= alarm->timePoint ) {
                    if( isDeviceArmed( alarm->deviceId.c_str() ) ) {
                        turnOnAlarms = true;
                    }
                    erase = true;
                }
                if( erase ) {
                    alarm = m_alarms.erase( alarm );
                }
                else {
                    ++alarm;
                }
            }
        }
    }
    if( turnOnAlarms ) {
        turnAllAlarmsOn();
    }
}

bool HouseModesManager::isDeviceModed( const std::string& deviceId,
                                       const char* deviceCat,
                                       const char* deviceSubCat ) const {
    if( deviceId.empty() || !deviceCat || !strlen( deviceCat ) ) {
        return false;
    }

    std::string category( deviceCat );
    std::string subCategory( deviceSubCat ? deviceSubCat : "" );

    for( auto& i : DefaultCatSubCat ) {
        if( category == i.cat && ( i.sub_cat == nullptr || subCategory == i.sub_cat ) ) {
            return true;
        }
    }

    return false;
}

bool HouseModesManager::isDeviceModed( const std::string& deviceId ) const {
    if( deviceId.empty() ) {
        return false;
    }

    std::vector<char> buff;
    {
        auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
        if( !devices ) {
            return false;
        }
        if( !devices->getBlob( deviceId, buff ) ) {
            return false;
        }
    }
    gateways::BaseDeviceInfo deviceInfo;
    gateways::ObjectSerializer serializer;
    if( !serializer.deserialize( deviceId, std::move( buff ), deviceInfo ) ) {
        return false;
    }

    return isDeviceModed( deviceId, deviceInfo.getCategory(), deviceInfo.getSubcategory() );
}

bool HouseModesManager::isDeviceArmed( const std::string& deviceId,
                                       const char* deviceCat,
                                       const char* deviceSubCat ) const {
    if( deviceId.empty() || !deviceCat || !strlen( deviceCat ) ) {
        return false;
    }

    if( !isDeviceModed( deviceId, deviceCat, deviceSubCat ) ) {
        return false;
    }

    auto it = std::find_if( m_sensorStates.begin(), m_sensorStates.end(), [deviceId]( const SensorState& device ) {
        return !strcmp( device.id.c_str(), deviceId.c_str() );
    } );
    if( it != m_sensorStates.end() ) {
        return it->armed;
    }

    std::string category( deviceCat );
    std::string subCategory( deviceSubCat ? deviceSubCat : "" );

    for( auto& i : m_presets ) {
        if( i.id == m_currentPreset ) {
            for( auto& device : i.sensorStates ) {
                if( !strcmp( device.id.c_str(), deviceId.c_str() ) ) {
                    return device.armed;
                }
            }
            if( i.id == HOME_MODE_ID ) {
                for( auto& j : HomeCatSubCat ) {
                    if( category == j.cat && ( j.sub_cat == nullptr || subCategory == j.sub_cat ) ) {
                        return true;
                    }
                }
                return false;
            }
            return true;
        }
    }

    return false;
}

bool HouseModesManager::isDeviceArmed( const std::string& deviceId ) const {
    if( deviceId.empty() ) {
        return false;
    }

    std::vector<char> buff;
    {
        auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
        if( !devices ) {
            return false;
        }
        if( !devices->getBlob( deviceId, buff ) ) {
            return false;
        }
    }
    gateways::BaseDeviceInfo deviceInfo;
    gateways::ObjectSerializer serializer;
    if( !serializer.deserialize( deviceId, std::move( buff ), deviceInfo ) ) {
        return false;
    }

    return isDeviceArmed( deviceId, deviceInfo.getCategory(), deviceInfo.getSubcategory() );
}

bool HouseModesManager::isAlarmDevice( const char* deviceCat, const char* deviceSubCat ) const {
    if( !deviceCat ) {
        return false;
    }
    for( auto& i : DefaultAlarmsCatSubCat ) {
        if( !std::strcmp( i.cat, deviceCat ) &&
            ( i.sub_cat == nullptr || !deviceSubCat || !std::strcmp( i.sub_cat, deviceSubCat ) ) ) {
            return true;
        }
    }
    return false;
}

bool HouseModesManager::isAlarmDeviceActivated( const char* deviceId ) const {
    bool result{ true };
    for( const auto& mode : m_presets ) {
        if( m_currentPreset == mode.id ) {
            if( std::any_of(
                    mode.alarmsOff.begin(), mode.alarmsOff.end(), [&]( const auto& id ) { return id == deviceId; } ) ) {
                result = false;
            }
        }
    }
    return result;
}

std::vector<HouseModesManager::SensorState> HouseModesManager::getSensorStates() const {
    std::vector<SensorState> sensorStates;
    if( Result::OK !=
        forDevices(
            [this]( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
                return isDeviceModed( deviceInfo.getId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() );
            },
            [&sensorStates, this]( const gateways::BaseDeviceInfo& deviceInfo ) -> void {
                sensorStates.emplace_back( SensorState{
                    deviceInfo.getId(),
                    isDeviceArmed( deviceInfo.getId(), deviceInfo.getCategory(), deviceInfo.getSubcategory() ) } );
            } ) ) {
        return {};
    }
    std::sort( sensorStates.begin(), sensorStates.end(), []( const SensorState& left, const SensorState& right ) {
        return left.id < right.id;
    } );
    return sensorStates;
}

HouseModesManager::Result HouseModesManager::sendArmedStateChangedBroadcasts(
    const std::vector<HouseModesManager::SensorState>& oldStates,
    const std::vector<HouseModesManager::SensorState>& newStates ) const {
    auto itOld = oldStates.cbegin();
    auto itNew = newStates.cbegin();
    while( itOld != oldStates.cend() && itNew != newStates.cend() ) {
        if( itOld->id == itNew->id ) {
            if( itOld->armed != itNew->armed ) {
                auto cmd = std::make_unique<lua::responce::UpdateDevice>();
                if( cmd ) {
                    cmd->m_deviceId = itOld->id;
                    cmd->m_key = common::resources::tokens::Armed;
                    cmd->m_value = std::make_unique<common::BoolValue>( itNew->armed );
                    send( lua::responce::BUS_NAME, std::move( cmd ) );
                }
                else {
                    FUNC_LOGE( "Failed to create bus command" );
                }
            }
            ++itOld;
            ++itNew;
        }
        else if( itOld->id < itNew->id ) {
            ++itOld;
        }
        else {
            ++itNew;
        }
    }
    return Result::OK;
}

void HouseModesManager::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddDevice& command ) {
    if( !command.m_deviceInfo ) {
        return;
    }
    bool added = false;
    for( auto& i : m_presets ) {
        auto j = std::find_if( i.sensorStates.begin(), i.sensorStates.end(), [&command]( const auto& device ) -> bool {
            return !strcmp( device.id.c_str(), command.m_deviceInfo->getId() );
        } );
        if( j == i.sensorStates.end() ) {
            if( i.id == HOME_MODE_ID ) {
                bool moded = false;
                bool armed = false;
                for( auto& k : DefaultCatSubCat ) {
                    if( command.m_deviceInfo->m_category == k.cat &&
                        ( k.sub_cat == nullptr || command.m_deviceInfo->m_subCategory == k.sub_cat ) ) {
                        moded = true;
                        break;
                    }
                }
                if( moded ) {
                    for( auto& k : HomeCatSubCat ) {
                        if( command.m_deviceInfo->m_category == k.cat &&
                            ( k.sub_cat == nullptr || command.m_deviceInfo->m_subCategory == k.sub_cat ) ) {
                            armed = true;
                        }
                    }
                }
                if( moded ) {
                    i.sensorStates.emplace_back( SensorState{ command.m_deviceInfo->getId(), armed } );
                }
            }
        }
        auto l = std::find_if( i.alarmsOff.begin(), i.alarmsOff.end(), [&command]( auto& iId ) -> bool {
            return iId == command.m_deviceInfo->getId();
        } );
        if( l == i.alarmsOff.end() ) {
            if( i.id == HOME_MODE_ID ) {
                bool moded = false;
                bool armed = false;
                for( auto& k : DefaultAlarmsCatSubCat ) {
                    if( command.m_deviceInfo->m_category == k.cat &&
                        ( k.sub_cat == nullptr || command.m_deviceInfo->m_subCategory == k.sub_cat ) ) {
                        moded = true;
                        break;
                    }
                }
                if( moded ) {
                    for( auto& k : HomeAlarmsCatSubCat ) {
                        if( command.m_deviceInfo->m_category == k.cat &&
                            ( k.sub_cat == nullptr || command.m_deviceInfo->m_subCategory == k.sub_cat ) ) {
                            armed = true;
                        }
                    }
                }
                if( moded && !armed ) {
                    i.alarmsOff.push_back( command.m_deviceInfo->getId() );
                }
            }
        }
    }
    if( added ) {
        save();
    }
}

void HouseModesManager::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::RemoveDevice& command ) {
    bool removed = false;
    for( auto& i : m_presets ) {
        auto j = std::find_if( i.sensorStates.begin(), i.sensorStates.end(), [&command]( const auto& device ) -> bool {
            return !strcmp( device.id.c_str(), command.m_deviceId.c_str() );
        } );
        if( j != i.sensorStates.end() ) {
            i.sensorStates.erase( j );
            removed = true;
        }
        auto k = std::find_if( i.alarmsOff.begin(), i.alarmsOff.end(), [&command]( auto& iId ) -> bool {
            return iId == command.m_deviceId;
        } );
        if( k != i.alarmsOff.end() ) {
            i.alarmsOff.erase( k );
            removed = true;
        }
    }
    if( removed ) {
        save();
    }
}

void HouseModesManager::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateItem& command ) {
    if( !command.m_itemInfo ) {
        FUNC_LOGE( "Command doesn't have info about the item" );
        return;
    }

    const auto& item = *command.m_itemInfo;
    const auto& deviceId = item.getDeviceId();
    bool alarm{ false };
    if( isDeviceArmed( deviceId ) && item.getName() == "security_threat" ) {
        auto boolVal = common::iValueCast<common::BoolValue>( command.value() );
        if( boolVal && boolVal->get() ) {
            alarm = true;
        }
    }
    if( alarm ) {
        std::lock_guard<std::mutex> lg( m_mtxAlarms );
        if( std::none_of(
                m_alarms.begin(), m_alarms.end(), [&]( const auto& alarm ) { return alarm.deviceId == deviceId; } ) ) {
            for( const auto& preset : m_presets ) {
                if( preset.id == m_currentPreset ) {
                    auto timePoint = std::chrono::steady_clock::now() + std::chrono::seconds( preset.alarmDelay );
                    FUNC_LOGI( "Add armed sensor to queue: %s", deviceId.c_str() );
                    m_alarms.emplace_back( AlarmTimePoint{ deviceId, timePoint } );
                    break;
                }
            }
        }
    }
}

HouseModesManager::Result HouseModesManager::getPresets( common::JsonStreamGenerator& result ) const {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    ok = ok && result.writeOpeningObject();
    char modeId[32];
    ok = ok && ( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_currentPreset ) > 0 );
    ok = ok && result.writeProperty( common::resources::tokens::Current, modeId );
    if( m_newPreset != PRESET_ID_INVALID && m_newPreset != PRESET_ID_CURRENT ) {
        ok = ok && ( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_newPreset ) > 0 );
        ok = ok && result.writeProperty( common::resources::tokens::SwitchTo, modeId );
    }
    else {
        ok = ok && result.writeProperty( common::resources::tokens::SwitchTo, "" );
    }

    auto now = std::chrono::steady_clock::now();
    if( m_newPreset != PRESET_ID_INVALID && m_newPreset != PRESET_ID_CURRENT && now < m_switchTimePoint ) {
        auto delta = std::chrono::duration_cast<std::chrono::duration<uint32_t> >( m_switchTimePoint - now );
        ok = ok && result.writeProperty( common::resources::tokens::SwitchToDelay, delta.count() );
    }
    else {
        ok = ok && result.writeProperty( common::resources::tokens::SwitchToDelay, 0 );
    }

    ok = ok && result.writePropertyKey( common::resources::tokens::Modes );
    ok = ok && result.writeOpeningArray();
    for( auto& preset : m_presets ) {
        ok = ok && result.writeOpeningObject();
        ok = ok && ( snprintf( modeId, sizeof( modeId ), "%" PRIu8, preset.id ) > 0 );
        ok = ok && result.writeProperty( common::resources::tokens::SubId, modeId );
        ok = ok && result.writeProperty( common::resources::tokens::Name, preset.name.c_str() );
        ok = ok && result.writeProperty( common::resources::tokens::Description, preset.description.c_str() );
        ok = ok && result.writeProperty( common::resources::tokens::SwitchToDelay, preset.switchToDelay );
        ok = ok && result.writeProperty( common::resources::tokens::AlarmDelay, preset.alarmDelay );
        if( preset.notifyAll ) {
            ok = ok && result.writeProperty( common::resources::tokens::Notifications, nullptr );
        }
        else {
            ok = ok && result.writePropertyKey( common::resources::tokens::Notifications );
            ok = ok && result.writeOpeningArray();
            for( auto& j : preset.notifications ) {
                ok = ok && result.writeProperty( nullptr, j.c_str() );
            }
            ok = ok && result.writeClosingSymbol();
        }
        ok = ok && result.writeProperty( common::resources::tokens::DisarmedDefault, preset.disarmedDefault );

        // disarmed devices
        ok = ok && result.writePropertyKey( common::resources::tokens::DisarmedDevices );
        ok = ok && result.writeOpeningArray();
        bool isHomeMode = preset.id == HOME_MODE_ID;

        ok = ok &&
             Result::OK == forDevices(
                               [=, &preset]( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
                                   const auto deviceId = deviceInfo.getId();
                                   for( const auto& device : preset.sensorStates ) {
                                       if( !strcmp( device.id.c_str(), deviceId ) ) {
                                           return !device.armed;
                                       }
                                   }
                                   if( isHomeMode ) {
                                       for( auto& j : HomeCatSubCat ) {
                                           if( deviceInfo.m_category == j.cat &&
                                               ( j.sub_cat == nullptr || deviceInfo.m_subCategory == j.sub_cat ) ) {
                                               return false;
                                           }
                                       }
                                   }
                                   for( auto& j : DefaultCatSubCat ) {
                                       if( deviceInfo.m_category == j.cat &&
                                           ( j.sub_cat == nullptr || deviceInfo.m_subCategory == j.sub_cat ) ) {
                                           return isHomeMode ? true : false;
                                       }
                                   }
                                   return false;
                               },
                               [&ok, &result]( const gateways::BaseDeviceInfo& deviceInfo ) -> void {
                                   ok = ok && result.writeProperty( nullptr, deviceInfo.getId() );
                               } );
        ok = ok && result.writeClosingSymbol();

        // ok = ok && result.writeProperty( common::resources::tokens::AlarmsOffDefault, i.alarmsOffDefault );
        ok = ok && result.writePropertyKey( common::resources::tokens::AlarmsOffDevices );
        ok = ok && result.writeOpeningArray();
        for( auto& j : preset.alarmsOff ) {
            ok = ok && result.writeProperty( nullptr, j.c_str() );
        }
        ok = ok && result.writeClosingSymbol();
        ok = ok && result.writeClosingSymbol();
    }
    ok = ok && result.writeClosingSymbol();
    // [ Devices
    ok = ok && result.writePropertyKey( common::resources::tokens::Devices );
    ok = ok && result.writeOpeningArray();
    ok = ok && ( HouseModesManager::Result::OK ==
                 forDevices(
                     []( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
                         for( auto& i : DefaultCatSubCat ) {
                             if( deviceInfo.m_category == i.cat &&
                                 ( i.sub_cat == nullptr || deviceInfo.m_subCategory == i.sub_cat ) ) {
                                 return true;
                             }
                         }
                         return false;
                     },
                     [&ok, &result]( const gateways::BaseDeviceInfo& deviceInfo ) -> void {
                         ok = ok && result.writeProperty( nullptr, deviceInfo.getId() );
                     } ) );
    ok = ok && result.writeClosingSymbol();
    // Devices ]
    // [ Alarms
    ok = ok && result.writePropertyKey( common::resources::tokens::Alarms );
    ok = ok && result.writeOpeningArray();
    ok = ok && ( HouseModesManager::Result::OK ==
                 forDevices(
                     [this]( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
                         return isAlarmDevice( deviceInfo.m_category.c_str(), deviceInfo.m_subCategory.c_str() );
                     },
                     [&ok, &result]( const gateways::BaseDeviceInfo& deviceInfo ) -> void {
                         ok = ok && result.writeProperty( nullptr, deviceInfo.getId() );
                     } ) );
    ok = ok && result.writeClosingSymbol();
    // Alarms ]
    ok = ok && result.writeClosingSymbol();  // }
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

HouseModesManager::PresetInfo HouseModesManager::getPresetInfo( std::string presetId ) const {
    PresetInfo presetInfo;
    auto id = std::atoi( presetId.c_str() );
    for( auto& preset : m_presets ) {
        if( preset.id != id ) {
            continue;
        }
        char modeId[32];
        snprintf( modeId, sizeof( modeId ), "%" PRIu8, preset.id );
        presetInfo.id = modeId;
        presetInfo.name = preset.name;
        presetInfo.defaultState = preset.disarmedDefault;

        int counter = 0;
        forDevices( []( const gateways::BaseDeviceInfo& deviceInfo ) -> bool { return true; },
                    [&counter]( const gateways::BaseDeviceInfo& deviceInfo ) -> void { counter++; } );
        presetInfo.disarmed_devices.reserve( counter );
        presetInfo.alarms_off_devices.reserve( counter );

        bool isHomeMode = preset.id == HOME_MODE_ID;
        forDevices(
            [=, &preset]( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
                const auto deviceId = deviceInfo.getId();
                for( const auto& device : preset.sensorStates ) {
                    if( !strcmp( device.id.c_str(), deviceId ) ) {
                        return !device.armed;
                    }
                }
                if( isHomeMode ) {
                    for( auto& j : HomeCatSubCat ) {
                        if( deviceInfo.m_category == j.cat &&
                            ( j.sub_cat == nullptr || deviceInfo.m_subCategory == j.sub_cat ) ) {
                            return false;
                        }
                    }
                }
                for( auto& j : DefaultCatSubCat ) {
                    if( deviceInfo.m_category == j.cat &&
                        ( j.sub_cat == nullptr || deviceInfo.m_subCategory == j.sub_cat ) ) {
                        return isHomeMode ? true : false;
                    }
                }
                return false;
            },
            [&presetInfo]( const gateways::BaseDeviceInfo& deviceInfo ) -> void {
                presetInfo.disarmed_devices.push_back( deviceInfo.getId() );
            } );

        for( auto& j : preset.alarmsOff ) {
            presetInfo.alarms_off_devices.push_back( j );
        }
    }

    return presetInfo;
}

HouseModesManager::PresetInfo HouseModesManager::findPresetByName( std::string name ) {
    PresetInfo presetInfo;

    for( auto& preset : m_presets ) {
        if( preset.name != name ) {
            continue;
        }
        char modeId[32];
        snprintf( modeId, sizeof( modeId ), "%" PRIu8, preset.id );
        presetInfo.id = modeId;
        presetInfo.name = preset.name;
        presetInfo.defaultState = preset.disarmedDefault;

        int counter = 0;
        forDevices( []( const gateways::BaseDeviceInfo& deviceInfo ) -> bool { return true; },
                    [&counter]( const gateways::BaseDeviceInfo& deviceInfo ) -> void { counter++; } );
        presetInfo.disarmed_devices.reserve( counter );
        presetInfo.alarms_off_devices.reserve( counter );

        bool isHomeMode = preset.id == HOME_MODE_ID;
        forDevices(
            [=, &preset]( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
                const auto deviceId = deviceInfo.getId();
                for( const auto& device : preset.sensorStates ) {
                    if( !strcmp( device.id.c_str(), deviceId ) ) {
                        return !device.armed;
                    }
                }
                if( isHomeMode ) {
                    for( auto& j : HomeCatSubCat ) {
                        if( deviceInfo.m_category == j.cat &&
                            ( j.sub_cat == nullptr || deviceInfo.m_subCategory == j.sub_cat ) ) {
                            return false;
                        }
                    }
                }
                for( auto& j : DefaultCatSubCat ) {
                    if( deviceInfo.m_category == j.cat &&
                        ( j.sub_cat == nullptr || deviceInfo.m_subCategory == j.sub_cat ) ) {
                        return isHomeMode ? true : false;
                    }
                }
                return false;
            },
            [&presetInfo]( const gateways::BaseDeviceInfo& deviceInfo ) -> void {
                presetInfo.disarmed_devices.push_back( deviceInfo.getId() );
            } );

        for( auto& j : preset.alarmsOff ) {
            presetInfo.alarms_off_devices.push_back( j );
        }
    }
    return presetInfo;
}

void HouseModesManager::getPresetsIds( std::vector<std::string>& presets ) const {
    presets.clear();
    for( auto& preset : m_presets ) {
        char modeId[32];
        if( 0 < snprintf( modeId, sizeof( modeId ), "%" PRIu8, preset.id ) ) {
            presets.push_back( modeId );
        }
    }
}

HouseModesManager::Result HouseModesManager::getCurrent( common::JsonStreamGenerator& result ) const {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    ok = ok && result.writeOpeningObject();
    char modeId[32];
    ok = ok && ( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_currentPreset ) > 0 );
    ok = ok && result.writeProperty( common::resources::tokens::ModeId, modeId );
    ok = ok && result.writeClosingSymbol();
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

uint8_t HouseModesManager::getCurrentPresetId() const {
    return m_currentPreset;
}

std::string HouseModesManager::currentPreset() const {
    char modeId[32];
    snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_currentPreset );
    return getPresetInfo( modeId ).id;
}

HouseModesManager::Result HouseModesManager::switchPreset( common::JsonStreamGenerator& result,
                                                           const HouseModesManager::PresetId presetId ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    ok = ok && result.writeOpeningObject();
    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else if( presetId == m_currentPreset ) {
        /// INFO: nothing todo
        ok = ok && result.writeProperty( common::resources::tokens::SwitchToDelay, 0 );
    }
    else if( presetId == m_newPreset ) {
        /// INFO: in progress
        auto delta = std::chrono::duration_cast<std::chrono::duration<uint32_t> >( m_switchTimePoint -
                                                                                   std::chrono::steady_clock::now() );
        ok = ok && result.writeProperty( common::resources::tokens::SwitchToDelay, delta.count() );
    }
    else {
        ret = switchPreset( presetId ) ? HouseModesManager::Result::OK : HouseModesManager::Result::NotFound;

        if( ret == HouseModesManager::Result::OK ) {
            for( const auto& preset : m_presets ) {
                if( preset.id == m_newPreset ) {
                    result.writeProperty( common::resources::tokens::SwitchToDelay, preset.switchToDelay );
                    break;
                }
            }
        }
    }
    ok = ok && result.writeClosingSymbol();
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

HouseModesManager::Result HouseModesManager::switchPreset( common::JsonStreamGenerator& result,
                                                           const std::string& name ) {
    HouseModesManager::PresetId preset_id = core::HouseModesManager::PRESET_ID_INVALID;
    for( const auto& preset : m_presets ) {
        if( !strcasecmp( preset.name.c_str(), name.c_str() ) ) {
            preset_id = preset.id;
            break;
        }
    }
    return switchPreset( result, preset_id );
}

bool HouseModesManager::switchPreset( const PresetId newPresetId ) {
    for( auto& presset : m_presets ) {
        if( newPresetId != presset.id ) {
            continue;
        }
        m_switchTimePoint = std::chrono::steady_clock::now() + std::chrono::seconds( presset.switchToDelay );
        m_newPreset = newPresetId;
        auto cmd = std::make_unique<bus::command::ModesSwitched>();
        if( cmd ) {
            char modeId[32];
            if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_currentPreset ) > 0 ) {
                cmd->m_from = modeId;
            }
            if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_newPreset ) > 0 ) {
                cmd->m_to = modeId;
            }
            cmd->m_status = "begin";
            cmd->m_switchDelay = presset.switchToDelay;
            send( "nma", std::move( cmd ) );
        }
        return true;
    }
    return false;
}

bool HouseModesManager::switchPreset( const std::string& presetId ) {
    if( presetId.empty() ) {
        return false;
    }
    auto index = std::atoi( presetId.c_str() );
    return switchPreset( index );
}

HouseModesManager::Result HouseModesManager::cancelSwitchPreset( common::JsonStreamGenerator& result ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    ok = ok && result.writeOpeningObject();
    char modeId[32];
    ok = ok && ( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_currentPreset ) > 0 );
    auto cmd = std::make_unique<bus::command::ModesSwitched>();
    if( cmd && m_newPreset != PRESET_ID_INVALID && m_newPreset != PRESET_ID_CURRENT &&
        m_newPreset != m_currentPreset ) {
        char modeId[32];
        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_currentPreset ) > 0 ) {
            cmd->m_from = modeId;
        }
        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_newPreset ) > 0 ) {
            cmd->m_to = modeId;
        }
        cmd->m_status = "cancel";
        cmd->m_switchDelay = 0;
        send( "nma", std::move( cmd ) );
    }
    m_newPreset = PRESET_ID_INVALID;  // reset switch
    ok = ok && result.writeProperty( common::resources::tokens::ModeId, modeId );
    ok = ok && result.writeClosingSymbol();
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

std::string HouseModesManager::cancelSwitchPreset() {
    auto cmd = std::make_unique<bus::command::ModesSwitched>();
    if( cmd && m_newPreset != PRESET_ID_INVALID && m_newPreset != PRESET_ID_CURRENT &&
        m_newPreset != m_currentPreset ) {
        char modeId[32];
        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_currentPreset ) > 0 ) {
            cmd->m_from = modeId;
        }
        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, m_newPreset ) > 0 ) {
            cmd->m_to = modeId;
        }
        cmd->m_status = "cancel";
        cmd->m_switchDelay = 0;
        send( "nma", std::move( cmd ) );
        return modeId;
    }
    m_newPreset = PRESET_ID_INVALID;  // reset switch
    return std::string();
}

HouseModesManager::Result HouseModesManager::setSwitchToDelay( common::JsonStreamGenerator& result, uint32_t delay ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    ok = ok && result.writeOpeningObject();
    for( auto& preset : m_presets ) {
        if( preset.id != HOME_MODE_ID ) {
            preset.switchToDelay = delay;
        }
    }
    ret = HouseModesManager::Result::OK;
    ok = ok && result.writeClosingSymbol();
    ok = ok && Result::OK == save();
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

HouseModesManager::Result HouseModesManager::setAlarmDelay( common::JsonStreamGenerator& result, uint32_t delay ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    ok = ok && result.writeOpeningObject();
    for( auto& i : m_presets ) {
        if( i.id != HOME_MODE_ID ) {
            i.alarmDelay = delay;
        }
    }
    ret = HouseModesManager::Result::OK;
    ok = ok && result.writeClosingSymbol();
    ok = ok && Result::OK == save();
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

HouseModesManager::Result HouseModesManager::setDefaultNotifications( PresetId presetId ) {
    for( auto& mode : m_presets ) {
        if( mode.id == presetId ) {
            auto notification = mode.notifications.begin();
            while( notification != mode.notifications.end() ) {
                if( !sendNotificationRemoveBroadcast( mode.id, *notification ) ) {
                    FUNC_LOGE( "Failed to create broadcast" );
                }
                notification = mode.notifications.erase( notification );
            }
            for( const auto& preset : defaultPresets ) {
                if( preset.id == mode.id ) {
                    if( mode.notifyAll != preset.notify_all ) {
                        mode.notifyAll = preset.notify_all;
                        if( !sendNotifyAllBroadcast( mode.id, mode.notifyAll ) ) {
                            FUNC_LOGE( "Failed to create broadcast" );
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
    return save();
}

HouseModesManager::Result HouseModesManager::setNotifications( common::JsonStreamGenerator& result,
                                                               PresetId presetId,
                                                               bool all,
                                                               std::vector<std::string>&& users ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                if( i.disarmedDefault ) {
                    i.disarmedDefault = false;
                    if( !sendModesChangedBroadcast( i.id,
                                                    common::resources::tokens::DisarmedDefault,
                                                    std::make_unique<common::BoolValue>( i.disarmedDefault ) ) ) {
                        FUNC_LOGE( "Failed to send modes changed broadcast" );
                    }
                }

                ret = HouseModesManager::Result::OK;
                if( all ) {
                    i.notifications.clear();
                    i.notifyAll = all;
                    ret = save();
                    if( Result::OK == ret ) {
                        if( !sendNotifyAllBroadcast( i.id, i.notifyAll ) ) {
                            FUNC_LOGE( "Failed to send notify all chnged broadcast" );
                        };
                    }
                    else {
                        ret = Result::SystemError;
                        FUNC_LOGE( "Failed to save settings" );
                    }
                }
                else {
                    if( i.notifyAll ) {
                        i.notifyAll = false;
                        if( !sendNotifyAllBroadcast( i.id, i.notifyAll ) ) {
                            FUNC_LOGE( "Failed to send notify all chnged broadcast" );
                        };
                    }
                    std::set<std::string> old_notifications;
                    for( auto& user : i.notifications ) {
                        old_notifications.emplace( std::move( user ) );
                    }
                    i.notifications.clear();
                    std::set<std::string> new_notifications;
                    for( auto& user : users ) {
                        i.notifications.emplace_back( user );
                        new_notifications.emplace( std::move( user ) );
                    }
                    std::set<std::string> diff;
                    std::set_difference( old_notifications.begin(),
                                         old_notifications.end(),
                                         new_notifications.begin(),
                                         new_notifications.end(),
                                         std::inserter( diff, diff.begin() ) );
                    for( const auto& user : diff ) {
                        if( !sendNotificationRemoveBroadcast( i.id, user ) ) {
                            FUNC_LOGE( "Failed to send notification removed broadcast" );
                        }
                    }
                    diff.clear();
                    std::set_difference( new_notifications.begin(),
                                         new_notifications.end(),
                                         old_notifications.begin(),
                                         old_notifications.end(),
                                         std::inserter( diff, diff.begin() ) );
                    for( const auto& user : diff ) {
                        if( !sendNotificationAddBroadcast( i.id, user ) ) {
                            FUNC_LOGE( "Failed to send notification removed broadcast" );
                        }
                    }
                    diff.clear();
                    ret = save();
                    if( Result::OK != ret ) {
                        FUNC_LOGE( "Failed to save settings" );
                    }
                }
                break;
            }
        }
    }
    return ret;
}

HouseModesManager::Result HouseModesManager::setNotificationAll( common::JsonStreamGenerator& result,
                                                                 PresetId presetId,
                                                                 bool all ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;

    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        ret = HouseModesManager::Result::NotFound;
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                if( i.disarmedDefault ) {
                    i.disarmedDefault = false;
                    if( !sendModesChangedBroadcast( i.id,
                                                    common::resources::tokens::DisarmedDefault,
                                                    std::make_unique<common::BoolValue>( i.disarmedDefault ) ) ) {
                        FUNC_LOGE( "Failed to send modes changed broadcast" );
                    }
                }
                ret = HouseModesManager::Result::OK;  // found
                if( i.notifyAll != all ) {
                    i.notifyAll = all;
                    if( Result::OK == save() ) {
                        auto cmd = std::make_unique<bus::command::ModesNotificationsNotifyAll>();
                        if( cmd ) {
                            char modeId[32];
                            if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) > 0 ) {
                                cmd->m_modeId = modeId;
                            }
                            cmd->m_all = all;
                            send( "nma", std::move( cmd ) );
                        }
                    }
                    else {
                        FUNC_LOGE( "Failed to save settings" );
                    }
                }
                break;
            }
        }
    }

    return ret;
}

HouseModesManager::Result HouseModesManager::addNotification( common::JsonStreamGenerator& result,
                                                              HouseModesManager::PresetId presetId,
                                                              const char* userId ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;

    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT || !userId || !strlen( userId ) ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                if( i.disarmedDefault ) {
                    i.disarmedDefault = false;
                    if( !sendModesChangedBroadcast( i.id,
                                                    common::resources::tokens::DisarmedDefault,
                                                    std::make_unique<common::BoolValue>( i.disarmedDefault ) ) ) {
                        FUNC_LOGE( "Failed to send modes changed broadcast" );
                    }
                }
                auto j = std::find_if( i.notifications.begin(), i.notifications.end(), [&userId]( auto& iId ) -> bool {
                    return iId == userId;
                } );
                if( j != i.notifications.end() ) {
                    // already exist
                }
                else {
                    i.notifications.push_back( userId );
                    save();
                    if( !sendNotificationAddBroadcast( presetId, userId ) ) {
                        FUNC_LOGE( "Failed to send broadcast" );
                    }
                }
                break;
            }
        }
    }

    return ret;
}

HouseModesManager::Result HouseModesManager::removeNotification( common::JsonStreamGenerator& result,
                                                                 HouseModesManager::PresetId presetId,
                                                                 const char* userId ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;

    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT || !userId || !strlen( userId ) ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                if( i.disarmedDefault ) {
                    i.disarmedDefault = false;
                    if( !sendModesChangedBroadcast( i.id,
                                                    common::resources::tokens::DisarmedDefault,
                                                    std::make_unique<common::BoolValue>( i.disarmedDefault ) ) ) {
                        FUNC_LOGE( "Failed to send modes changed broadcast" );
                    }
                }
                auto j = std::find_if( i.notifications.begin(), i.notifications.end(), [&userId]( auto& iId ) -> bool {
                    return iId == userId;
                } );
                if( j != i.notifications.end() ) {
                    j = i.notifications.erase( j );
                    save();
                    if( !sendNotificationRemoveBroadcast( presetId, userId ) ) {
                        FUNC_LOGE( "Failed to send broadcast" );
                    }
                }
                else {
                    // not found
                    ret = HouseModesManager::Result::NotFound;
                }
                break;
            }
        }
    }

    return ret;
}

HouseModesManager::Result HouseModesManager::getCurrentNotifications( common::JsonStreamGenerator& result ) const {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    for( auto& i : m_presets ) {
        if( i.id == m_currentPreset ) {
            if( i.notifyAll ) {
                ok = ok && result.writePropertyValue( nullptr );
            }
            else {
                ok = ok && result.writeOpeningArray();
                for( auto& j : i.notifications ) {
                    ok = ok && result.writeProperty( nullptr, j.c_str() );
                }
                ok = ok && result.writeClosingSymbol();
            }
        }
    }
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

HouseModesManager::Result HouseModesManager::setDisarmedDefault( common::JsonStreamGenerator& result,
                                                                 HouseModesManager::PresetId presetId,
                                                                 bool def ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;

    bool sendDeviceUpdate = m_currentPreset == presetId;
    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        ret = HouseModesManager::Result::NotFound;
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                ret = HouseModesManager::Result::OK;  // found
                if( i.disarmedDefault != def ) {
                    std::vector<SensorState> oldSensorStates{};
                    if( sendDeviceUpdate ) {
                        oldSensorStates = getSensorStates();
                    }
                    i.disarmedDefault = def;
                    if( !sendModesChangedBroadcast( i.id,
                                                    common::resources::tokens::DisarmedDefault,
                                                    std::make_unique<common::BoolValue>( i.disarmedDefault ) ) ) {
                        FUNC_LOGE( "Failed to send modes changed broadcast" );
                    }
                    if( i.disarmedDefault ) {
                        i.sensorStates.clear();
                    }
                    save();
                    auto cmd = std::make_unique<bus::command::ModesDisarmedDevicesDefault>();
                    if( cmd ) {
                        char modeId[32];
                        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) > 0 ) {
                            cmd->m_modeId = modeId;
                        }
                        cmd->m_disarmedDefault = def;
                        send( "nma", std::move( cmd ) );
                    }
                    if( sendDeviceUpdate ) {
                        sendArmedStateChangedBroadcasts( oldSensorStates, getSensorStates() );
                    }
                }
                break;
            }
        }
    }

    return ret;
}

HouseModesManager::Result HouseModesManager::addDisarmedDevice( common::JsonStreamGenerator& result,
                                                                HouseModesManager::PresetId presetId,
                                                                const char* deviceId ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;

    bool oldDeviceState{ false };
    bool sendDeviceUpdate = presetId == m_currentPreset;
    if( sendDeviceUpdate ) {
        oldDeviceState = isDeviceArmed( deviceId );
    }
    ok = ok && result.writeOpeningObject();
    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT || !deviceId || !strlen( deviceId ) ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                if( i.disarmedDefault ) {
                    i.disarmedDefault = false;
                    if( !sendModesChangedBroadcast( i.id,
                                                    common::resources::tokens::DisarmedDefault,
                                                    std::make_unique<common::BoolValue>( i.disarmedDefault ) ) ) {
                        FUNC_LOGE( "Failed to send modes changed broadcast" );
                    }
                }
                auto j = std::find_if(
                    i.sensorStates.begin(), i.sensorStates.end(), [&deviceId]( const auto& device ) -> bool {
                        return !strcmp( device.id.c_str(), deviceId );
                    } );
                bool sendDisarmedBroadcast = false;
                if( j != i.sensorStates.end() ) {
                    if( j->armed ) {
                        j->armed = false;
                        sendDisarmedBroadcast = true;
                        save();
                    }
                }
                else {
                    i.sensorStates.emplace_back( SensorState{ deviceId, false } );
                    sendDisarmedBroadcast = true;
                    save();
                }
                if( sendDisarmedBroadcast ) {
                    auto cmd = std::make_unique<bus::command::ModesDisarmedDevicesAdded>();
                    if( cmd ) {
                        char modeId[32];
                        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) > 0 ) {
                            cmd->m_modeId = modeId;
                        }
                        cmd->m_disarmedDevice = deviceId;
                        send( "nma", std::move( cmd ) );
                    }
                    if( sendDeviceUpdate ) {
                        bool newDeviceState = isDeviceArmed( deviceId );
                        if( oldDeviceState != newDeviceState ) {
                            auto cmd = std::make_unique<lua::responce::UpdateDevice>();
                            if( cmd ) {
                                cmd->m_deviceId = deviceId;
                                cmd->m_key = common::resources::tokens::Armed;
                                cmd->m_value = std::make_unique<common::BoolValue>( newDeviceState );
                                send( lua::responce::BUS_NAME, std::move( cmd ) );
                            }
                            else {
                                FUNC_LOGE( "Failed to create bus command" );
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    ok = ok && result.writeClosingSymbol();
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

HouseModesManager::Result HouseModesManager::removeDisarmedDevice( common::JsonStreamGenerator& result,
                                                                   HouseModesManager::PresetId presetId,
                                                                   const char* deviceId ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    bool oldDeviceState{ true };
    bool sendDeviceUpdate = presetId == m_currentPreset;
    if( sendDeviceUpdate ) {
        oldDeviceState = isDeviceArmed( deviceId );
    }
    ok = ok && result.writeOpeningObject();
    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT || !deviceId || !strlen( deviceId ) ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else if( !isDeviceModed( deviceId ) ) {
        ret = HouseModesManager::Result::NotFound;
    }
    else {
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                if( i.disarmedDefault ) {
                    i.disarmedDefault = false;
                    if( !sendModesChangedBroadcast( i.id,
                                                    common::resources::tokens::DisarmedDefault,
                                                    std::make_unique<common::BoolValue>( i.disarmedDefault ) ) ) {
                        FUNC_LOGE( "Failed to send modes changed broadcast" );
                    }
                }
                bool sendDisarmedRemovedBroadcast = false;
                auto j = std::find_if(
                    i.sensorStates.begin(), i.sensorStates.end(), [&deviceId]( const auto& device ) -> bool {
                        return !strcmp( device.id.c_str(), deviceId );
                    } );
                if( j != i.sensorStates.end() ) {
                    if( !j->armed ) {
                        j->armed = true;
                        sendDisarmedRemovedBroadcast = true;
                        save();
                    }
                }
                else {
                    i.sensorStates.emplace_back( SensorState{ deviceId, true } );
                    sendDisarmedRemovedBroadcast = true;
                    save();
                }
                if( sendDisarmedRemovedBroadcast ) {
                    auto cmd = std::make_unique<bus::command::ModesDisarmedDevicesRemoved>();
                    if( cmd ) {
                        char modeId[32];
                        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) > 0 ) {
                            cmd->m_modeId = modeId;
                        }
                        cmd->m_disarmedDevice = deviceId;
                        send( "nma", std::move( cmd ) );
                    }
                    if( sendDeviceUpdate ) {
                        bool newDeviceState = isDeviceArmed( deviceId );
                        if( oldDeviceState != newDeviceState ) {
                            auto cmd = std::make_unique<lua::responce::UpdateDevice>();
                            if( cmd ) {
                                cmd->m_deviceId = deviceId;
                                cmd->m_key = common::resources::tokens::Armed;
                                cmd->m_value = std::make_unique<common::BoolValue>( newDeviceState );
                                send( lua::responce::BUS_NAME, std::move( cmd ) );
                            }
                            else {
                                FUNC_LOGE( "Failed to create bus command" );
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    ok = ok && result.writeClosingSymbol();
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

HouseModesManager::Result HouseModesManager::setAlarmsOffDefault( common::JsonStreamGenerator& result,
                                                                  HouseModesManager::PresetId presetId,
                                                                  bool def ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    bool ok = true;
    ok = ok && result.writeOpeningObject();
    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        ret = HouseModesManager::Result::NotFound;
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                ret = HouseModesManager::Result::OK;  // found
                if( i.alarmsOffDefault != def ) {
                    i.alarmsOffDefault = def;
                    save();
                    auto cmd = std::make_unique<bus::command::ModesAlarmsOffDefault>();
                    if( cmd ) {
                        char modeId[32];
                        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) > 0 ) {
                            cmd->m_modeId = modeId;
                        }
                        cmd->m_alarmsOffDefault = def;
                        send( "nma", std::move( cmd ) );
                    }
                }
                break;
            }
        }
    }
    ok = ok && result.writeClosingSymbol();
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }
    return ret;
}

HouseModesManager::Result HouseModesManager::addAlarmsOff( common::JsonStreamGenerator& result,
                                                           HouseModesManager::PresetId presetId,
                                                           const char* deviceId ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;

    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT || !deviceId || !strlen( deviceId ) ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                if( i.disarmedDefault ) {
                    i.disarmedDefault = false;
                    if( !sendModesChangedBroadcast( i.id,
                                                    common::resources::tokens::DisarmedDefault,
                                                    std::make_unique<common::BoolValue>( i.disarmedDefault ) ) ) {
                        FUNC_LOGE( "Failed to send modes changed broadcast" );
                    }
                }
                auto j = std::find_if( i.alarmsOff.begin(), i.alarmsOff.end(), [&deviceId]( auto& iId ) -> bool {
                    return iId == deviceId;
                } );
                if( j != i.alarmsOff.end() ) {
                    // already exist
                }
                else {
                    i.alarmsOff.push_back( deviceId );
                    save();
                    auto cmd = std::make_unique<bus::command::ModesAlarmsOffAdded>();
                    if( cmd ) {
                        char modeId[32];
                        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) > 0 ) {
                            cmd->m_modeId = modeId;
                        }
                        cmd->m_alarmOff = deviceId;
                        send( "nma", std::move( cmd ) );
                    }
                }
                break;
            }
        }
    }

    return ret;
}

HouseModesManager::Result HouseModesManager::removeAlarmsOff( common::JsonStreamGenerator& result,
                                                              HouseModesManager::PresetId presetId,
                                                              const char* deviceId ) {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;

    if( presetId == PRESET_ID_INVALID || presetId == PRESET_ID_CURRENT || !deviceId || !strlen( deviceId ) ) {
        ret = HouseModesManager::Result::WrongArguments;
    }
    else {
        for( auto& i : m_presets ) {
            if( presetId == i.id ) {
                auto j = std::find_if( i.alarmsOff.begin(), i.alarmsOff.end(), [&deviceId]( auto& iId ) -> bool {
                    return iId == deviceId;
                } );
                if( j != i.alarmsOff.end() ) {
                    j = i.alarmsOff.erase( j );
                    save();
                    auto cmd = std::make_unique<bus::command::ModesAlarmsOffRemoved>();
                    if( cmd ) {
                        char modeId[32];
                        if( snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) > 0 ) {
                            cmd->m_modeId = modeId;
                        }
                        cmd->m_alarmOff = deviceId;
                        send( "nma", std::move( cmd ) );
                    }
                }
                else {
                    // not found
                    ret = HouseModesManager::Result::NotFound;
                }
                break;
            }
        }
    }

    return ret;
}

HouseModesManager::Result HouseModesManager::setDeviceArmed( const char* deviceId, bool armed, bool* prev_state ) {
    if( !deviceId ) {
        return Result::WrongArguments;
    }
    if( !isDeviceModed( deviceId ) ) {
        FUNC_LOGW( "Device cannot be armed/disarmed" );
        return Result::NotSupported;
    }
    bool oldDeviceState = isDeviceArmed( deviceId );
    if( prev_state ) {
        *prev_state = oldDeviceState;
    }
    auto it = std::find_if( m_sensorStates.begin(), m_sensorStates.end(), [deviceId]( const SensorState& device ) {
        return !strcmp( device.id.c_str(), deviceId );
    } );
    if( it != m_sensorStates.end() ) {
        it->armed = armed;
    }
    else {
        m_sensorStates.emplace_back( SensorState{ deviceId, armed } );
    }
    bool newDeviceState = isDeviceArmed( deviceId );
    if( oldDeviceState != newDeviceState ) {
        auto cmd = std::make_unique<lua::responce::UpdateDevice>();
        if( cmd ) {
            cmd->m_deviceId = deviceId;
            cmd->m_key = common::resources::tokens::Armed;
            cmd->m_value = std::make_unique<common::BoolValue>( newDeviceState );
            send( lua::responce::BUS_NAME, std::move( cmd ) );
        }
        else {
            FUNC_LOGE( "Failed to create bus command" );
        }
    }
    return save();
}

HouseModesManager::Result HouseModesManager::save() const {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    storage::File file;
    if( !file.open( PresetsFile, "wb" ) ) {
        return HouseModesManager::Result::SystemError;
    }

    bool ok = file.write( StorageVersion );
    ok = file.write( defaultVersion );  // version of default presets
    ok = ok && file.write( static_cast<uint8_t>( m_presets.size() ) );
    ok = ok && file.write( static_cast<uint8_t>( m_currentPreset ) );  // current
    for( auto& i : m_presets ) {
        ok = ok && file.write( static_cast<uint8_t>( i.id ) );
        ok = ok && file.writeString( i.name.c_str() );
        ok = ok && file.writeString( i.description.c_str() );
        ok = ok && file.write( i.switchToDelay );
        ok = ok && file.write( i.alarmDelay );
        ok = ok && file.write( static_cast<uint8_t>( i.notifyAll ) );
        ok = ok && file.write( static_cast<uint32_t>( i.notifications.size() ) );
        for( auto& j : i.notifications ) {
            ok = ok && file.writeString( j.c_str() );
        }
        ok = ok && file.write( static_cast<uint8_t>( i.disarmedDefault ) );
        ok = ok && file.write( static_cast<uint32_t>( i.sensorStates.size() ) );
        for( const auto& device : i.sensorStates ) {
            ok = ok && file.writeString( device.id.c_str() );
            ok = ok && file.write( device.armed );
        }
        ok = ok && file.write( static_cast<uint8_t>( i.alarmsOffDefault ) );
        ok = ok && file.write( static_cast<uint32_t>( i.alarmsOff.size() ) );
        for( auto& j : i.alarmsOff ) {
            ok = ok && file.writeString( j.c_str() );
        }
    }
    if( !ok ) {
        ret = HouseModesManager::Result::SystemError;
    }

    ok = ok && file.write( static_cast<uint8_t>( m_sensorStates.size() ) );
    for( const auto& device : m_sensorStates ) {
        ok = ok && file.writeString( device.id.c_str() );
        ok = ok && file.write( device.armed );
    }

    return ret;
}

HouseModesManager::Result HouseModesManager::load() {
    HouseModesManager::Result ret = HouseModesManager::Result::OK;
    storage::File file;

    m_presets.clear();

    uint16_t ver = 0;
    bool ok = file.open( PresetsFile, "rb" ) && file.read( ver ) && ( ver == StorageVersion );

    uint16_t ver2 = 0;
    ok = ok && file.read( ver2 );

    uint8_t count = 0;
    ok = ok && file.read( count );
    ok = ok && file.read( m_currentPreset );
    for( size_t i = 0; i < count; ++i ) {
        m_presets.emplace_back();
        auto& preset = m_presets.back();

        std::string name;
        std::string description;
        ok = ok && file.read( preset.id );
        ok = ok && file.readString( name );
        ok = ok && file.readString( description );
        if( ok ) {
            preset.name = name;
            preset.description = description;
        }
        ok = ok && file.read( preset.switchToDelay );
        ok = ok && file.read( preset.alarmDelay );
        uint8_t notifyAll = !0;
        ok = ok && file.read( notifyAll );
        preset.notifyAll = notifyAll;
        uint32_t notifyCount = 0;
        ok = ok && file.read( notifyCount );
        for( size_t j = 0; j < notifyCount; ++j ) {
            std::string userId;
            ok = ok && file.readString( userId );
            if( ok ) {
                preset.notifications.push_back( userId );
            }
        }
        uint8_t disDefault = !0;
        ok = ok && file.read( disDefault );
        preset.disarmedDefault = disDefault;
        uint32_t armedCount = 0;
        ok = ok && file.read( armedCount );
        for( size_t j = 0; j < armedCount; ++j ) {
            std::string deviceId;
            ok = ok && file.readString( deviceId );
            bool armed;
            ok = ok && file.read( armed );
            if( ok ) {
                preset.sensorStates.emplace_back( SensorState{ deviceId, armed } );
            }
        }
        uint8_t aDefault = !0;
        ok = ok && file.read( aDefault );
        preset.alarmsOffDefault = aDefault;
        uint32_t aCount = 0;
        ok = ok && file.read( aCount );
        for( size_t j = 0; j < aCount; ++j ) {
            std::string deviceId;
            ok = ok && file.readString( deviceId );
            if( ok ) {
                preset.alarmsOff.push_back( deviceId );
            }
        }
    }
    uint8_t n_customArmed = 0u;
    ok = ok && file.read( n_customArmed );
    for( uint8_t n = 0u; n < n_customArmed; ++n ) {
        std::string deviceId;
        ok = ok && file.readString( deviceId );
        bool armed;
        ok = ok && file.read( armed );
        if( ok ) {
            m_sensorStates.emplace_back( SensorState{ deviceId, armed } );
        }
    }

    if( !ok ) {
        // reset to default
        file.close();
        m_presets.clear();
        m_currentPreset = defaultPresets[0].id;
        for( auto& i : defaultPresets ) {
            m_presets.emplace_back();
            auto& preset = m_presets.back();
            preset.id = i.id;
            preset.name = i.name;
            preset.description = i.description;
            preset.switchToDelay = i.switch_to_delay;
            preset.alarmDelay = i.alarm_delay;
            preset.notifyAll = i.notify_all;
            if( preset.id == HOME_MODE_ID ) {
                forDevices(  // alarms
                    []( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
                        for( auto& i : DefaultAlarmsCatSubCat ) {
                            if( deviceInfo.m_category == i.cat &&
                                ( i.sub_cat == nullptr || deviceInfo.m_subCategory == i.sub_cat ) ) {
                                return true;
                            }
                        }
                        return false;
                    },
                    [&preset]( const gateways::BaseDeviceInfo& deviceInfo ) -> void {
                        for( auto& i : HomeAlarmsCatSubCat ) {
                            if( deviceInfo.m_category == i.cat &&
                                ( i.sub_cat == nullptr || deviceInfo.m_subCategory == i.sub_cat ) ) {
                                return;
                            }
                        }
                        preset.alarmsOff.push_back( deviceInfo.getId() );
                    } );
            }
        }
        m_sensorStates.clear();
        ret = save();
    }
    else if( defaultVersion == ver2 ) {
        /// TODO: ...
    }

    return ret;
}

void HouseModesManager::turnAllAlarmsOn() const {
    FUNC_LOGI( "Turn on all sirens" );
    struct AlarmItems {
        std::string deviceId;
        std::array<std::string, sizeof( PreferredAlarmItemTypes ) / sizeof( PreferredAlarmItemTypes[0] )> items;
    };
    std::list<AlarmItems> alarmDevices;
    forDevices(
        [this]( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
            return isAlarmDevice( deviceInfo.m_category.c_str(), deviceInfo.m_subCategory.c_str() ) &&
                   isAlarmDeviceActivated( deviceInfo.getId() );
        },
        [this, &alarmDevices]( const gateways::BaseDeviceInfo& deviceInfo ) -> void {
            alarmDevices.emplace_back( AlarmItems{ deviceInfo.getId(), {} } );
        } );

    {
        auto items = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
        if( !items ) {
            FUNC_LOGE( "Failed to open store" );
            return;
        }

        for( auto it = items->cbegin(); it != items->cend(); ++it ) {
            auto val = it.value();
            if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ) {
                FUNC_LOGE( "Failed to get a current value for the key: %s", ( *it ).c_str() );
                continue;
            }
            gateways::ItemInfo itemInfo;
            gateways::ObjectSerializer serializer;
            if( !serializer.deserialize( std::move( val.get<BlobType>() ), itemInfo ) ) {
                FUNC_LOGE( "Failed to deserialize the key: %s", ( *it ).c_str() );
                continue;
            }
            itemInfo.setId( ( *it ).c_str() );
            for( auto& device : alarmDevices ) {
                bool found{ false };
                if( device.deviceId == itemInfo.getDeviceId() ) {
                    int index{ 0 };
                    for( auto type : PreferredAlarmItemTypes ) {
                        if( type == itemInfo.getName() ) {
                            device.items[index] = itemInfo.getId();
                            found = true;
                            break;
                        }
                        ++index;
                    }
                }
                if( found ) {
                    break;
                }
            }
        }
    }

    for( const auto& siren : alarmDevices ) {
        for( const auto& id : siren.items ) {
            if( !id.empty() ) {
                auto pluginName = gateways::utils::getPluginNameByItemId( id.c_str() );
                send<bus::command::CloudItemValueSet>(
                    "debug", pluginName, id, std::make_unique<common::IntValue>( 1 ) );
                break;
            }
        }
    }
}

bool HouseModesManager::sendNotifyAllBroadcast( PresetId presetId, bool all ) const {
    char modeId[32];
    if( !snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) ) {
        return false;
    }
    auto cmd = std::make_unique<bus::command::ModesNotificationsNotifyAll>();
    if( !cmd ) {
        return false;
    }
    cmd->m_modeId = modeId;
    cmd->m_all = all;
    send( "nma", std::move( cmd ) );
    return true;
}

bool HouseModesManager::sendNotificationRemoveBroadcast( PresetId presetId, const std::string& id ) const {
    char modeId[32];
    if( !snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) ) {
        return false;
    }
    auto cmd = std::make_unique<bus::command::ModesNotificationsRemoved>();
    if( !cmd ) {
        return false;
    }
    cmd->m_modeId = modeId;
    cmd->m_notification = id;
    send( "nma", std::move( cmd ) );
    return true;
}

bool HouseModesManager::sendNotificationAddBroadcast( PresetId presetId, const std::string& id ) const {
    char modeId[32];
    if( !snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) ) {
        return false;
    }
    auto cmd = std::make_unique<bus::command::ModesNotificationsAdded>();
    if( !cmd ) {
        return false;
    }
    cmd->m_modeId = modeId;
    cmd->m_notification = id;
    send( "nma", std::move( cmd ) );
    return true;
}

bool HouseModesManager::sendModesChangedBroadcast( PresetId presetId,
                                                   const std::string& name,
                                                   std::unique_ptr<common::IValue> value ) const {
    char modeId[32];
    if( !snprintf( modeId, sizeof( modeId ), "%" PRIu8, presetId ) ) {
        return false;
    }
    auto cmd = std::make_unique<bus::command::ModesChanged>();
    if( !cmd ) {
        return false;
    }
    cmd->m_modeId = modeId;
    cmd->m_properties.emplace_back( std::make_pair( name, std::move( value ) ) );
    send( "nma", std::move( cmd ) );
    return true;
}

bool HouseModesManager::isNotificationSendToUser( const gateways::ItemInfo& itemInfo, const common::IValue& value ) {
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

    bool is_armed = isDeviceArmed( itemInfo.getDeviceId() );

    if( is_armed && !strcmp( deviceInfo.getCategory(), "security_sensor" ) &&
        !strcmp( deviceInfo.getSubcategory(), "motion" ) && ( itemInfo.getName() == "security_threat" ) ) {
        return true;
    }

    if( !strcmp( deviceInfo.getCategory(), "security_sensor" ) && ( itemInfo.getName() == "alarm_intrusion_tamper" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && strVal->get() == "tampering_cover" ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "battery" ) ) {
        if( !strcmp( deviceInfo.getCategory(), "door_lock" ) ||
            !strcmp( deviceInfo.getCategory(), "security_sensor" ) || !strcmp( deviceInfo.getCategory(), "humidity" ) ||
            !strcmp( deviceInfo.getCategory(), "light_sensor" ) ||
            !strcmp( deviceInfo.getCategory(), "temperature" ) ) {
            auto intVal = common::iValueCast<common::IntValue>( &value );
            if( intVal && intVal->get() <= 20 ) {
                return true;
            }
        }
    }

    if( ( itemInfo.getName() == "alarm_smoke" ) || ( itemInfo.getName() == "alarm_sensor_smoke" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && strVal->get() == "smoke_detected" ) {
            return true;
        }
    }

    if( !strcmp( deviceInfo.getCategory(), "door_lock" ) && ( itemInfo.getName() == "alarm_access_control" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && strVal->get() == "unlock_by_rf_invalid_user_code" ) {
            return true;
        }
    }

    if( !strcmp( deviceInfo.getCategory(), "security_sensor" ) && !strcmp( deviceInfo.getSubcategory(), "door" ) &&
        is_armed && ( ( itemInfo.getName() == "sensor_binary" ) || itemInfo.getName() == "security_threat" ) ) {
        const auto boolVal = common::iValueCast<common::BoolValue>( &value );
        if( boolVal && boolVal->get() ) {
            return true;
        }
    }

    if( !strcmp( deviceInfo.getCategory(), "security_sensor" ) && itemInfo.getName() == "tampering_cover_alarm" ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && ( strVal->get() == "tampering_cover" || strVal->get() == "no_tampering_cover" ) ) {
            return true;
        }
    }

    if( is_armed && ( itemInfo.getName() == "dw_state" ) && is_armed ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && strVal->get() == "dw_is_closed" ) {
            return true;
        }
    }

    if( !strcmp( deviceInfo.getCategory(), "security_sensor" ) && !strcmp( deviceInfo.getSubcategory(), "leak" ) &&
        is_armed && ( itemInfo.getName() == "security_threat" ) ) {
        auto boolVal = common::iValueCast<common::BoolValue>( &value );
        if( boolVal && boolVal->get() ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "water_leak_alarm" ) && is_armed ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && strVal->get() == "no_water_leak" ) {
            return true;
        }
    }

    if( !strcmp( deviceInfo.getCategory(), "security_sensor" ) && !strcmp( deviceInfo.getSubcategory(), "motion" ) &&
        ( itemInfo.getName() == "motion" ) && is_armed ) {
        return true;
    }

    if( ( itemInfo.getName() == "smoke_alarm" ) && is_armed ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && ( strVal->get() == "no_smoke" || strVal->get() == "smoke_detected" ) ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "co_alarm" ) && is_armed ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && ( strVal->get() == "no_co" || strVal->get() == "co_detected" ) ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "glass_breakage_alarm" ) && is_armed ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && strVal->get() == "glass_breakage" ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "water_temperature_alarm" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && ( strVal->get() == "water_temperature_below_low_threshold" ||
                        strVal->get() == "water_temperature_above_high_threshold" ) ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "lock_operation" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal &&
            ( strVal->get() == "unlock_by_rf_with_invalid_user_code" ||
              strVal->get() == "lock_by_rf_with_invalid_user_code" || strVal->get() == "manual_lock_operation" ||
              strVal->get() == "keypad_lock_operation" || strVal->get() == "auto_lock_not_fully_locked_operation" ||
              strVal->get() == "manual_not_fully_locked_operation" ||
              strVal->get() == "rf_not_fully_locked_operation" || strVal->get() == "lock_jammed" ||
              strVal->get() == "rf_lock_operation" || strVal->get() == "rf_unlock_operation" ||
              strVal->get() == "manual_unlock_operation" || strVal->get() == "keypad_unlock_operation" ) ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "user_code_operation" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && ( strVal->get() == "new_program_code_entered" ||
                        strVal->get() == "new_user_code_not_added_due_to_duplicated_code" ||
                        strVal->get() == "new_user_code_added" ||
                        strVal->get() == "manually_enter_user_code_exceeds_code_limit" ||
                        strVal->get() == "single_user_code_deleted" || strVal->get() == "all_user_codes_deleted" ) ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "thermostat_mode" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && ( strVal->get() == "off" || strVal->get() == "auto" || strVal->get() == "heat" ||
                        strVal->get() == "cool" ) ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "thermostat_fan_mode" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal &&
            ( strVal->get() == "low" || strVal->get() == "high" || strVal->get() == "medium" ||
              strVal->get() == "auto_low" || strVal->get() == "auto_medium" || strVal->get() == "auto_high" ) ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "switch" ) ) {
        auto boolVal = common::iValueCast<common::BoolValue>( &value );
        if( boolVal ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "barrier_state" ) ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && ( strVal->get() == "barrier_opened" || strVal->get() == "barrier_closed" ) ) {
            return true;
        }
    }

    if( ( itemInfo.getName() == "heat_alarm" ) && !strcmp( deviceInfo.getCategory(), "security_sensor" ) &&
        !strcmp( deviceInfo.getSubcategory(), "heat" ) && is_armed ) {
        auto strVal = common::iValueCast<common::StringValue>( &value );
        if( strVal && ( ( strVal->get() == "overheat_detected" ) || ( strVal->get() == "under_heat_detected" ) ) ) {
            return true;
        }
    }

    return needPowerStateSensorItemNotification( itemInfo, value );
}

bool HouseModesManager::needPowerStateSensorItemNotification( const gateways::ItemInfo& itemInfo,
                                                              const common::IValue& value ) {
    auto checkNameWithValue = [itemInfo, &value]( const std::string& key, const char* val ) -> bool {
        if( ( itemInfo.getName() == key ) ) {
            auto strVal = common::iValueCast<common::StringValue>( &value );
            if( strVal && ( strVal->get() == val ) ) {
                return true;
            }
        }

        return false;
    };

    return checkNameWithValue( "power_surge_state", "surge_detected" ) ||
           checkNameWithValue( "voltage_drop_drift_state", "drop_drift_detected" ) ||
           checkNameWithValue( "over_current_state", "over_current_detected" ) ||
           checkNameWithValue( "over_voltage_state", "over_voltage_detected" ) ||
           checkNameWithValue( "over_load_state", "over_load_detected" ) ||
           checkNameWithValue( "load_error_state", "load_error" ) ||
           checkNameWithValue( "battery_maintenance_state", "replace_battery_soon" ) ||
           checkNameWithValue( "battery_maintenance_state", "replace_battery_now" ) ||
           checkNameWithValue( "battery_maintenance_state", "battery_fluid_is_low" ) ||
           checkNameWithValue( "battery_charging_state", "in_progress" ) ||
           checkNameWithValue( "ac_state", "connected" );
}

std::string HouseModesManager::parentDeviceName( const gateways::ItemInfo& itemInfo, const common::IValue& value ) {
    std::string res;

    if( !needPowerStateSensorItemNotification( itemInfo, value ) ) {
        return std::string();
    }

    auto deviceId = itemInfo.getDeviceId();

    if( deviceId.empty() ) {
        return std::string();
    }

    std::string parentDeviceId;
    forDevices(
        [this, deviceId, &parentDeviceId]( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
            if( deviceInfo.getId() == deviceId ) {
                parentDeviceId = deviceInfo.getParentDeviceId();
                return true;
            }

            return false;
        },
        []( const gateways::BaseDeviceInfo& deviceInfo ) -> void {} );

    forDevices(
        [this, parentDeviceId, &res]( const gateways::BaseDeviceInfo& deviceInfo ) -> bool {
            if( deviceInfo.getId() == parentDeviceId ) {
                res = deviceInfo.getName();
                return true;
            }

            return false;
        },
        []( const gateways::BaseDeviceInfo& deviceInfo ) -> void {} );

    return res;
}

}  // namespace core
