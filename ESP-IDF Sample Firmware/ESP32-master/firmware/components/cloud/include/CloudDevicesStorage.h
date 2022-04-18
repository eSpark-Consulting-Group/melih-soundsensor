
#ifdef CONFIG_CLOUD_DEVICES

#include <GatewayDefs.h>
#include <devices/ZWaveDevice.h>
#include <IGateway.h>

namespace storage {
class File;
}  // namespace storage

namespace gateways {
namespace cloud {

class BaseDeviceInfo;
class Device;
struct ItemData;

class DevicesStorage final {
    static constexpr size_t MaxStorageBufferSize = 512;

public:
    using DevicePtr = std::shared_ptr<gateways::cloud::Device>;

    static DevicesStorage& get();

public:
    bool checkVersion( const char version );

    bool readVersion( char& version );

    bool storeVersion( const char version );

    bool addDeviceFile( std::unique_ptr<gateways::cloud::Device>& device, const char version );

    bool removeDeviceFile( const std::string& deviceId );

    bool restoreAndCheckVersion( storage::File& file ) const;

    Device restoreDevice( const std::string& id, IGateway::AddingDeviceStatus& addingStatus );

private:
    DevicesStorage() = default;

    bool writeVersionHeader( storage::File& file, const char version ) const;
};

}  // namespace cloud
}  // namespace gateways

#endif  // CONFIG_CLOUD_DEVICES
