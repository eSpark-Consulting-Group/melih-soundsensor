
#pragma once

#include <cinttypes>

namespace gateways {

enum class ThermostatOperatingStates {
    Idle = 0,
    Heating,
    Cooling,
    FanOnly,
    PendingHeat,
    PendingCool,
    VentEco,
    AuxHeating,
    SecStageHeating,
    SecStageCooling,
    SecStageAuxHeat,
    ThirdStageAuxHeat,
    Unknown
};

ThermostatOperatingStates convertStringToThermostatOperatingState( const char* opstate );

const char* toEnumValue( ThermostatOperatingStates opstate );

}  // namespace gateways
