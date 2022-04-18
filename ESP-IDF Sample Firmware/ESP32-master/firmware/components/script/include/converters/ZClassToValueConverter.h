#pragma once

#include <znet/cc_0x20_basic.hpp>
#include <znet/cc_0x25_switch.hpp>
#include <znet/cc_0x26_switch_multi_level.hpp>
#include <znet/cc_0x2b_scene_activation.hpp>
#include <znet/cc_0x2c_scene_actuator_conf.hpp>
#include <znet/cc_0x30_binary_sensor.hpp>
#include <znet/cc_0x31_sensor_multilevel.hpp>
#include <znet/cc_0x32_meter.hpp>
#include <znet/cc_0x33_color_switch.hpp>
#include <znet/cc_0x40_thermostat_mode.hpp>
#include <znet/cc_0x42_thermostat_operating_state.hpp>
#include <znet/cc_0x43_thermostat_setpoint.hpp>
#include <znet/cc_0x44_thermostat_fan_mode.hpp>
#include <znet/cc_0x45_thermostat_fan_state.hpp>
#include <znet/cc_0x47_thermostat_setback.hpp>
#include <znet/cc_0x4e_schedule_entry_lock.hpp>
#include <znet/cc_0x59_association_grp_info.hpp>
#include <znet/cc_0x5b_central_scene.hpp>
#include <znet/cc_0x5e_zwave_plus_info.hpp>
#include <znet/cc_0x5d_anti_theft.hpp>
#include <znet/cc_0x60_multi_channel.hpp>
#include <znet/cc_0x62_door_lock.hpp>
#include <znet/cc_0x63_user_code.hpp>
#include <znet/cc_0x66_barrier_operator.hpp>
#include <znet/cc_0x6f_entry_control.hpp>
#include <znet/cc_0x70_configuration.hpp>
#include <znet/cc_0x71_notification.hpp>
#include <znet/cc_0x72_manufacturer_specific.hpp>
#include <znet/cc_0x79_sound_switch.hpp>
#include <znet/cc_0x7a_firmware_update.hpp>
#include <znet/cc_0x7e_anti_theft_unlock.hpp>
#include <znet/cc_0x80_battery.hpp>
#include <znet/cc_0x81_clock.hpp>
#include <znet/cc_0x84_wakeup.hpp>
#include <znet/cc_0x85_association.hpp>
#include <znet/cc_0x86_version.hpp>
#include <znet/cc_0x87_indicator.hpp>
#include <znet/cc_0x8a_time.hpp>
#include <znet/cc_0x8b_time_parameters.hpp>
#include <znet/cc_0x8e_multi_channel_association.hpp>
#include <znet/cc_0x98_security.hpp>
#include <znet/cc_0x9c_alarm_sensor.hpp>
#include <znet/cc_0x9d_alarm_silence.hpp>
#include <znet/cc_0x9f_security2.hpp>
#include <znet/cc_0x87_indicator.hpp>

namespace common {
class TableValue;
}

namespace lua {
namespace module {
namespace converters {
namespace zwave {

/**
 * Converts Z-Wave Command Class description to ::common::TableValue.
 * The result table will contain at least id/version fields.
 */
class ZClassToValueConverter {
public:
    explicit ZClassToValueConverter(::common::TableValue& value );

    void operator()(::znet::ZClass& cc );

    void operator()(::znet::ZCc0x20Basic& cc );
    void operator()(::znet::ZCc0x30BinarySensor& cc );
    void operator()(::znet::ZCc0x31SensorMultilevel& cc );
    void operator()(::znet::ZCc0x32Meter& cc );
    void operator()(::znet::ZCc0x33ColorSwitch& cc );
    void operator()(::znet::ZCc0x40ThermostatMode& cc );
    void operator()(::znet::ZCc0x43ThermostatSetpoint& cc );
    void operator()(::znet::ZCc0x44ThermostatFanMode& cc );
    void operator()(::znet::ZCc0x5BCentralScene& cc );
    void operator()(::znet::ZCc0x63UserCode& cc );
    void operator()(::znet::ZCc0x71Notification& cc );
    void operator()(::znet::ZCc0x9cAlarmSensor& cc );
    void operator()(::znet::ZCc0x79SoundSwitch& cc );
    void operator()(::znet::ZCc0x4EScheduleEntryLock& cc );
    void operator()(::znet::ZCc0x7AFirmwareUpdate& cc );
    void operator()(::znet::ZCc0x86Version& cc );
    void operator()(::znet::ZCc0x66BarrierOperator& cc );
    void operator()(::znet::ZCc0x70Configuration& cc );
    void operator()(::znet::ZCc0x5DAntiTheft& cc );
    void operator()(::znet::ZCc0x7EAntiTheftUnlock& cc );
    void operator()(::znet::ZCc0x62DoorLock& cc );

    void operator()(::znet::ZCcAntiTheftBase& cc );

private:
    template <typename T>
    void addOptionalField( const char* name, znet::utils::optional<T> value );

    ::common::TableValue& m_value;
};

}  // namespace zwave
}  // namespace converters
}  // namespace module
}  // namespace lua
