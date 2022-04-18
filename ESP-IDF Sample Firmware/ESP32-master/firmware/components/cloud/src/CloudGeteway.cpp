#ifdef CONFIG_CLOUD_DEVICES

#include <CloudGateway.h>

#include <Defs.h>
#include <DebugUtils.h>

#include <GatewayDeviceRestore.h>
#include <GatewayUtils.h>
#include <ColudDevicesManager.h>
#include <JsonStreamGenerator.h>
#include <values/FloatValue.h>
#include <DeviceSettingsManager.h>
#include <devices/CloudDevice.h>
#include "lua/responce/RemoveDevice.h"
#include <strings.h>
#include <algorithm>
#include <chrono>

namespace gateways {

namespace cloud {

const int GetAbstractsListInterval = 50000;

CloudGateway& CloudGateway::get() {
    static CloudGateway res;
    return res;
}

GatewayID CloudGateway::getTypeId() const {
    return GatewayID::Cloud;
}

CloudGateway::ClbkRet CloudGateway::updateComponent( UpdContext& ctx ) {
    ColudDevicesManager::get().onLoop();
    return ClbkRet::Continue;
}

void CloudGateway::startup( ScriptExecutor scriptExecutor ) {
    ColudDevicesManager::get().startup( scriptExecutor );
}

size_t CloudGateway::getRecommendedThreadStackSize() const {
    return 4096;
}

bool CloudGateway::initialize() {
    if( !ColudDevicesManager::get().initialize() ) {
        return false;
    }

    return true;
}

CloudGateway::AddingDeviceStatus CloudGateway::restoreDeviceInfo( const DeviceId&, ::storage::File& ) {
    return AddingDeviceStatus::OK;
}

void CloudGateway::cleanAllData() {}

CloudGateway::DevicePtr CloudGateway::getDevice( std::string deviceId ) {
    auto itFind = std::find_if( m_devices.begin(), m_devices.end(), [&deviceId]( const DevicePtr& device ) {
        return device->getId() == deviceId;
    } );
    return itFind == m_devices.end() ? DevicePtr() : *itFind;

    return DevicePtr();
}

}  // namespace cloud
}  // namespace gateways

#endif  // CONFIG_CLOUD_DEVICES
