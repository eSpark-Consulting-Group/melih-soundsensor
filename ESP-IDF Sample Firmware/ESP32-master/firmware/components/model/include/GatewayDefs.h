
#pragma once

#include <cinttypes>

namespace gateways {

using DeviceId = uint32_t;
using ItemId = uint32_t;

constexpr auto DeviceFileExtensionSymbol = 'D';
constexpr auto DeviceFileExtensionSize = 2u;
constexpr auto MinimalDeviceFileNameSize = DeviceFileExtensionSize + 2;

constexpr auto DeviceSettingsFileExtensionSymbol = 'P';
constexpr auto DeviceSettingsFileExtensionSize = 2u;
constexpr auto MinimalDeviceSettingsFileNameSize = DeviceFileExtensionSize + 2;

}  // namespace gateways
