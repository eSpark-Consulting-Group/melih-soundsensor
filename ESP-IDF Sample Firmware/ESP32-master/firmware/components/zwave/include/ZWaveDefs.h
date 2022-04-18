#pragma once

#include <cinttypes>

namespace gateways {
namespace zwave {

constexpr int DeviceIdMaxSize = 256;
constexpr uint32_t EachDeviceReachableCheckTimeout = 86'400;  // One day
constexpr uint32_t AllDevicesReachableCheckTimeout = 10'800;  // 3 hours

}  // namespace zwave
}  // namespace gateways
