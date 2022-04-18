#pragma once

#include <cstdint>

namespace bus {

/**
 * @brief Bus addres type
 *
 * addr == 0 - invalid address
 * addr < 0 - client address
 * addr > 0 - group address
 */
typedef std::size_t BusAddr;
constexpr BusAddr BusAddrInvalid = 0;

}  // namespace bus
