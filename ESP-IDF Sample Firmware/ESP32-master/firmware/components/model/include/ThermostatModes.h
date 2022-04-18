
#pragma once

#include <cinttypes>

namespace gateways {

enum class ThermostatModes {
    Off = 0,
    Heat,
    Cool,
    Auto,
    AuxiliaryHeat,
    Resume,
    FanOnly,
    Furnace,
    DryAir,
    MoistAir,
    AutoChangeover,
    EnergySaveHeat,
    EnergySaveCool,
    Away,
    Reserved,
    FullPower,
    Unknown
};

ThermostatModes convertStringToThermostatModes( const char* mode );

const char* toEnumValue( ThermostatModes mode );

}  // namespace gateways
