#pragma once

#include "devices/ZWaveDevice.h"

namespace common {
class TableValue;
}

namespace lua {
namespace module {
namespace converters {
namespace zwave {

/**
 * Converts Z-Wave device to generic ::common::TableValue.
 * Only top-level device properties are converted, i.e. "channels" is not included.
 */
class DeviceToValueConverter {
public:
    explicit DeviceToValueConverter(::common::TableValue& value );

    void operator()( const ::gateways::zwave::devices::Device& );

private:
    ::common::TableValue& m_value;
};

}  // namespace zwave
}  // namespace converters
}  // namespace module
}  // namespace lua
