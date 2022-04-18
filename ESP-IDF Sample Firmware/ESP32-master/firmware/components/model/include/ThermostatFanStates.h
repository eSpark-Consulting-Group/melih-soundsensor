
#pragma once

#include <cinttypes>

namespace gateways {

enum class ThermostatFanStates {
    IdleOff = 0,
    RunningLow,
    RunningHigh,
    RunningMedium,
    CirculationMode,
    HumidityCirculationMode,
    RightLeftCirculationMode,
    UpDownCirculationMode,
    QuietCirculationMode,
    Unknown
};

ThermostatFanStates convertStringToThermostatFanState( const char* fanstate );

const char* toEnumValue( ThermostatFanStates fanstate );

}  // namespace gateways
