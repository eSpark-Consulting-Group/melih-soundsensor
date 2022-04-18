
#ifdef CONFIG_CLOUD_DEVICES

#include <DeviceInfo.h>
#include <devices/CloudDevice.h>
#include <GatewayID.h>
#include <IGateway.h>

namespace gateways {
namespace cloud {

class CloudGateway : public IGateway {
public:
    using DevicePtr = std::shared_ptr<Device>;
    using Devices = std::vector<DevicePtr>;

    static CloudGateway& get();

public:
    GatewayID getTypeId() const override;
    void startup( ScriptExecutor ) override;
    ClbkRet updateComponent( UpdContext& ctx ) override;
    size_t getRecommendedThreadStackSize() const override;
    bool initialize() override;
    AddingDeviceStatus restoreDeviceInfo( const DeviceId&, ::storage::File& ) override;
    void cleanAllData() override;

public:
    DevicePtr getDevice( std::string deviceId );

private:
    CloudGateway() = default;
    Devices m_devices;
};

}  // namespace cloud

}  // namespace gateways

#endif  // CONFIG_CLOUD_DEVICES
