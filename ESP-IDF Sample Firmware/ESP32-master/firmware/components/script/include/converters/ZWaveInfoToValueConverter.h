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
 * Converts Z-Wave gateway to generic ::common::TableValue.
 */
class ZWaveInfoToValueConverter {
public:
    explicit ZWaveInfoToValueConverter(::common::TableValue& value );

    void operator()();

private:
    ::common::TableValue& m_value;
};

}  // namespace zwave
}  // namespace converters
}  // namespace module
}  // namespace lua
