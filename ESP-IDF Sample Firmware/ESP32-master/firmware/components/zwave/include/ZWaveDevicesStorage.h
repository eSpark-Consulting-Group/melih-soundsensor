
#pragma once

#include <GatewayDefs.h>
#include <devices/ZWaveDevice.h>
#include <IGateway.h>

namespace storage {
class File;
}  // namespace storage

namespace gateways {
namespace zwave {
namespace devices {

class BaseDeviceInfo;
class Device;
struct ItemData;

}  // namespace devices

class DevicesStorage final {
    static constexpr size_t MaxStorageBufferSize = 512;

public:
    using DevicePtr = std::shared_ptr<devices::Device>;

    static DevicesStorage& get();

public:
    bool addDeviceFile( devices::Device* device );

    bool removeDeviceFile( uint8_t uniqueId );

    bool restoreAndCheckVersion( storage::File& file ) const;

    std::unique_ptr<devices::BaseDeviceInfo> restoreDeviceInfo( DevicePtr&& device );

    DevicePtr restoreDevice( uint8_t deviceId, storage::File& file, IGateway::AddingDeviceStatus& addingStatus );

private:
    DevicesStorage() = default;

    bool writeVersionHeader( storage::File& file ) const;
};

}  // namespace zwave
}  // namespace gateways
