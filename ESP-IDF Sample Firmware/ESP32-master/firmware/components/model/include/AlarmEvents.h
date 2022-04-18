
#pragma once

#include <cinttypes>

namespace gateways {

enum class AlarmEventsCo {
    Unknown = 0,

    NoCo,

    CoWarning,
    CoDetected,
    CoDetectedUnknownLocation,
    CoDetectedTest,
    CoAlarmTestOk,
    CoAlarmTestFailed
};

enum class AlarmEventsCo2 {
    Unknown = 0,

    NoCo2,

    Co2Warning,
    Co2Detected,
    Co2DetectedUnknownLocation,
    Co2DetectedTest,
    Co2AlarmTestOk,
    Co2AlarmTestFailed
};

const char* toEnumValue( AlarmEventsCo mode );
const char* toEnumValue( AlarmEventsCo2 mode );

}  // namespace gateways
