
#pragma once

#include <cinttypes>

namespace gateways {

enum class ThermostatFanModes {
    AutoLow = 0,
    Low,
    AutoHigh,
    High,
    AutoMedium,
    Medium,
    Circulation,
    HumidityCirculation,
    LeftAndRight,
    UpAndDown,
    Quiet,
    Unknown
};

ThermostatFanModes convertStringToThermostatFanMode( const char* fanmode );

const char* toEnumValue( ThermostatFanModes fanmode );

}  // namespace gateways
