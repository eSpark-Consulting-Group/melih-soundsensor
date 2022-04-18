
#pragma once

#include <cinttypes>

namespace gateways {

enum class AlarmSensorEventsSmoke {
    Unknown = 0,

    NoSmoke,
    SmokeWarning,
    SmokeDetected
};

const char* toEnumValue( AlarmSensorEventsSmoke mode );

}  // namespace gateways
