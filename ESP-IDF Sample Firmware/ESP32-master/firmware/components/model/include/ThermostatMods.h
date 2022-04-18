
#pragma once

#include <cinttypes>

namespace gateways {

enum class ThermostatMods {
    Off = 0,
    Cool,
    Heat,
    Auto,
    HeatCool = Auto,
    AuxiliaryHeat,
    Eco = AuxiliaryHeat,
    Unknown,
};

ThermostatMods convertStringToThermostatMode( const char* mode );

const char* toEnumValue( ThermostatMods mode );

}  // namespace gateways
