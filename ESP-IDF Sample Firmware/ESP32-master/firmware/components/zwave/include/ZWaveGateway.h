
#pragma once

#include <ZWaveDefs.h>
#include <ZWaveZNet.h>
#include <NmaCommand.h>
#include <DeviceInfo.h>
#include <devices/ZWaveDevice.h>
#include <GatewayID.h>
#include <IGateway.h>
#include <Volume.h>
#include <stdarg.h>
#include <list>
#include <znet/znet.hpp>

namespace gateways {
namespace zwave {
namespace devices {
class BaseDeviceInfo;
}

class Gateway : public IGateway {
public:
    using DevicePtr = std::shared_ptr<devices::Device>;
    using Devices = std::vector<DevicePtr>;

    static Gateway& get();

    ~Gateway() override;

public:
    GatewayID getTypeId() const override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    size_t getRecommendedThreadStackSize() const override;

    bool initialize() override;

    void startup( ScriptExecutor scriptExecutor ) override;

    IGateway::AddingDeviceStatus restoreDeviceInfo( const DeviceId& deviceId, storage::File& file ) override;

    void cleanAllData() override;

public:
    bool checkReachableDevicesState();

public:
    size_t getDevicesCount();

    DevicePtr getDeviceByIndex( size_t idx );

    DevicePtr getDevice( uint8_t deviceId );

    DevicePtr addDevice( uint8_t deviceId );

    bool removeDevice( uint8_t deviceId );

private:
    Gateway();

private:
    uint32_t m_lastDeviceStatusCheckingTime = 0;

    Devices m_devices;
};

}  // namespace zwave
}  // namespace gateways
