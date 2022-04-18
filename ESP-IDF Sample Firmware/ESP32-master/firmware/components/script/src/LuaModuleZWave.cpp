#include "LuaModuleZWave.h"

#include <mbedtls/base64.h>

#include "bindings/LuaZWaveBindings.hpp"
#include "bindings/LuaBindingHelper.h"

#include "lua/BusCommandSendEventToLuaHandler.hpp"

namespace lua {
namespace module {

bool ZWave::initialize() {
    return true;
}

ZWave::ZWave( Executor& exec, const char* name )
: IModule( exec, name ) {}

int ZWave::regModule( lua_State* lua ) {
    static const auto bindArray = lua::createBindingsMap(
        luaL_Reg{ "join", bindings::luaBinding<bindings::zwave::Join> },
        luaL_Reg{ "subscribe", bindings::luaBinding<bindings::zwave::Subscribe> },
        luaL_Reg{ "unsubscribe", bindings::luaBinding<bindings::zwave::Unsubscribe> },

        luaL_Reg{ "add_configuration", bindings::luaBinding<bindings::zwave::AddConfiguration> },

        luaL_Reg{ "check_node_values", bindings::luaBinding<bindings::zwave::CheckNodeValues> },
        luaL_Reg{ "check_reachable_status", bindings::luaBinding<bindings::zwave::CheckReachableStatus> },

        luaL_Reg{ "get_all_nodes_ids", bindings::luaBinding<bindings::zwave::GetAllNodesIds> },
        luaL_Reg{ "get_background_rssi", bindings::luaBinding<bindings::zwave::GetBackgroundRSSI> },
        luaL_Reg{ "get_command_class", bindings::luaBinding<bindings::zwave::GetCommandClass> },
        luaL_Reg{ "get_firmware_region", bindings::luaBinding<bindings::zwave::GetFirmwareRegion> },
        luaL_Reg{ "get_info", bindings::luaBinding<bindings::zwave::GetInfo> },
        luaL_Reg{ "get_node", bindings::luaBinding<bindings::zwave::GetNode> },
        luaL_Reg{ "get_node_info", bindings::luaBinding<bindings::zwave::GetNodeInfo> },
        luaL_Reg{ "get_manufacturer_name", bindings::luaBinding<bindings::zwave::GetManufacturerName> },

        luaL_Reg{ "has_node", bindings::luaBinding<bindings::zwave::HasNode> },

        luaL_Reg{ "reconstruct_network", bindings::luaBinding<bindings::zwave::ReconstructNetwork> },
        luaL_Reg{ "request_barrier_signal_value", bindings::luaBinding<bindings::zwave::RequestBarrierSignalValue> },
        luaL_Reg{ "request_configuration_value", bindings::luaBinding<bindings::zwave::RequestConfigurationValue> },
        luaL_Reg{ "request_door_lock_configuration",
                  bindings::luaBinding<bindings::zwave::RequestDoorLockConfiguration> },
        luaL_Reg{ "request_door_lock_state", bindings::luaBinding<bindings::zwave::RequestDoorLockState> },
        luaL_Reg{ "request_node_values", bindings::luaBinding<bindings::zwave::RequestNodeValues> },
        luaL_Reg{ "request_value", bindings::luaBinding<bindings::zwave::RequestValue> },
        luaL_Reg{ "request_user_code", bindings::luaBinding<bindings::zwave::RequestUserCode> },
        luaL_Reg{ "request_master_code", bindings::luaBinding<bindings::zwave::RequestMasterCode> },
        luaL_Reg{ "request_user_codes_keypad_mode", bindings::luaBinding<bindings::zwave::RequestUsesCodesKeypadMode> },
        luaL_Reg{ "request_user_codes_checksum", bindings::luaBinding<bindings::zwave::RequestUserCodesChecksum> },
        luaL_Reg{ "request_week_day_entry_lock_schedule",
                  bindings::luaBinding<bindings::zwave::RequestWeekDayEntryLockSchedule> },
        luaL_Reg{ "request_daily_repeating_entry_lock_schedule",
                  bindings::luaBinding<bindings::zwave::RequestDailyRepeatingEntryLockSchedule> },
        luaL_Reg{ "request_year_day_entry_lock_schedule",
                  bindings::luaBinding<bindings::zwave::RequestYearDayEntryLockSchedule> },
        luaL_Reg{ "request_setpoint_value", bindings::luaBinding<bindings::zwave::RequestSetpointValue> },
        luaL_Reg{ "request_sound_switch_tone_play", bindings::luaBinding<bindings::zwave::RequestSoundSwitchTonePlay> },
        luaL_Reg{ "request_sound_switch_configuration",
                  bindings::luaBinding<bindings::zwave::RequestSoundSwitchConfiguration> },
        luaL_Reg{ "request_alarm_v1_value", bindings::luaBinding<bindings::zwave::RequestAlarmV1Value> },
        luaL_Reg{ "remove_configuration", bindings::luaBinding<bindings::zwave::RemoveConfiguration> },
        luaL_Reg{ "reset", bindings::luaBinding<bindings::zwave::Reset> },
        luaL_Reg{ "reset_configuration_value", bindings::luaBinding<bindings::zwave::ResetConfigurationValue> },
        luaL_Reg{ "reset_all_configuration_values",
                  bindings::luaBinding<bindings::zwave::ResetAllConfigurationValues> },

        luaL_Reg{ "lock_anti_theft", bindings::luaBinding<bindings::zwave::LockAntiTheft> },
        luaL_Reg{ "lock_anti_theft_default", bindings::luaBinding<bindings::zwave::LockAntiTheftDefault> },
        luaL_Reg{ "unlock_anti_theft", bindings::luaBinding<bindings::zwave::UnlockAntiTheft> },
        luaL_Reg{ "unlock_anti_theft_default", bindings::luaBinding<bindings::zwave::UnlockAntiTheftDefault> },

        luaL_Reg{ "set_auth_mode", bindings::luaBinding<bindings::zwave::SetAuthMode> },
        luaL_Reg{ "set_barrier_signal_value", bindings::luaBinding<bindings::zwave::SetBarrierSignalValue> },
        luaL_Reg{ "set_configuration_value", bindings::luaBinding<bindings::zwave::SetConfigurationValue> },
        luaL_Reg{ "set_device_specific_key", bindings::luaBinding<bindings::zwave::SetDeviceSpecificKey> },
        luaL_Reg{ "set_devices_poll_intervals", bindings::luaBinding<bindings::zwave::SetDevicesPollIntervals> },
        luaL_Reg{ "set_default_device_configurations_file",
                  bindings::luaBinding<bindings::zwave::SetDefaultConfigurtionFile> },
        luaL_Reg{ "set_door_lock_mode", bindings::luaBinding<bindings::zwave::SetDoorLockMode> },
        luaL_Reg{ "set_door_lock_configuration", bindings::luaBinding<bindings::zwave::SetDoorLockConfiguration> },
        luaL_Reg{ "set_rgb_value", bindings::luaBinding<bindings::zwave::SetRgbValue> },
        luaL_Reg{ "set_rgb_value_multicast", bindings::luaBinding<bindings::zwave::SetRgbValueMulticast> },
        luaL_Reg{ "set_setpoint_value", bindings::luaBinding<bindings::zwave::SetSetpointValue> },
        luaL_Reg{ "set_thermostat_setback_value", bindings::luaBinding<bindings::zwave::SetThermostatSetbackValue> },
        luaL_Reg{ "set_thermostat_mode", bindings::luaBinding<bindings::zwave::SetThermostatMode> },
        luaL_Reg{ "set_token_value", bindings::luaBinding<bindings::zwave::SetValue> },
        luaL_Reg{ "set_token_value_multicast", bindings::luaBinding<bindings::zwave::SetValueMulticast> },
        luaL_Reg{ "set_value", bindings::luaBinding<bindings::zwave::SetValue> },
        luaL_Reg{ "set_value_multicast", bindings::luaBinding<bindings::zwave::SetValueMulticast> },
        luaL_Reg{ "set_user_code", bindings::luaBinding<bindings::zwave::SetUserCode> },
        luaL_Reg{ "set_master_code", bindings::luaBinding<bindings::zwave::SetMasterCode> },
        luaL_Reg{ "set_user_codes_keypad_mode", bindings::luaBinding<bindings::zwave::SetUserCodesKeypadMode> },
        luaL_Reg{ "set_week_day_entry_lock_schedule",
                  bindings::luaBinding<bindings::zwave::SetWeekDayEntryLockSchedule> },
        luaL_Reg{ "set_daily_repeating_entry_lock_schedule",
                  bindings::luaBinding<bindings::zwave::SetDailyRepeatingEntryLockSchedule> },
        luaL_Reg{ "set_year_day_entry_lock_schedule",
                  bindings::luaBinding<bindings::zwave::SetYearDayEntryLockSchedule> },
        luaL_Reg{ "set_sound_switch_tone_play", bindings::luaBinding<bindings::zwave::SetSoundSwitchTonePlay> },
        luaL_Reg{ "set_sound_switch_configuration",
                  bindings::luaBinding<bindings::zwave::SetSoundSwitchConfiguration> },
        luaL_Reg{ "switch_multilevel_level_change", bindings::luaBinding<bindings::zwave::SetSwitchMultilevelLevel> },
        luaL_Reg{ "color_switch_level_change", bindings::luaBinding<bindings::zwave::SetColorSwitchLevel> },

        luaL_Reg{ "start_include", bindings::luaBinding<bindings::zwave::StartInclude> },
        luaL_Reg{ "start_exclude", bindings::luaBinding<bindings::zwave::StartExclude> },
        luaL_Reg{ "start_learn", bindings::luaBinding<bindings::zwave::StartLearn> },
        luaL_Reg{ "start_rediscover", bindings::luaBinding<bindings::zwave::StartRediscover> },

        luaL_Reg{ "stop_include", bindings::luaBinding<bindings::zwave::StopInclude> },
        luaL_Reg{ "stop_security_include", bindings::luaBinding<bindings::zwave::StopSecurityInclude> },
        luaL_Reg{ "stop_exclude", bindings::luaBinding<bindings::zwave::StopExclude> },  // add to doc
        luaL_Reg{ "stop_learn", bindings::luaBinding<bindings::zwave::StopLearn> },

        luaL_Reg{ "set_provisioning_info", bindings::luaBinding<bindings::zwave::SetProvisioningInfo> },
        luaL_Reg{ "get_provisioning_info", bindings::luaBinding<bindings::zwave::GetProvisioningInfo> },

        luaL_Reg{ "send_identify", bindings::luaBinding<bindings::zwave::SendIdentify> },

        luaL_Reg{ "calculate_user_codes_checksum", bindings::luaBinding<bindings::zwave::CalculateUserCodesChecksum> },

        luaL_Reg{ "update_nodes_firmware", bindings::luaBinding<bindings::zwave::UpdateNodesFirmware> } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

void ZWave::visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::SendGenericEventToLuaHandler& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }
    ValueToLuaGenerator gen{ lua };
    gen.visit( command.getEvent() );
    m_exec.luaRet( 1 );
}

}  // namespace module

}  // namespace lua
