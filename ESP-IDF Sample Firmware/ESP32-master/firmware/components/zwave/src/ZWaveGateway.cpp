
#include <ZWaveGateway.h>

#include <Defs.h>
#include <DebugUtils.h>

#include <GatewayDeviceRestore.h>
#include <GatewayUtils.h>
#include <ZWaveDevicesStorage.h>
#include <devices/ZWaveDevice.h>
#include <ZWaveZNet.h>
#include <ResetZW.h>

#include <JsonStreamGenerator.h>
#include <values/FloatValue.h>
#include <values/TableValue.h>
#include <DeviceSettingsManager.h>

#include "lua/responce/RemoveDevice.h"

#include <strings.h>
#include <algorithm>

namespace gateways {
namespace zwave {

using namespace std::chrono_literals;

Gateway& Gateway::get() {
    static Gateway zwave;
    return zwave;
}

Gateway::Gateway() {
    m_devices.reserve( 0 );
}

Gateway::~Gateway() = default;

GatewayID Gateway::getTypeId() const {
    return GatewayID::ZWave;
}

Gateway::ClbkRet Gateway::updateComponent( UpdContext& ctx ) {
    while( ZNet::get().onLoop() && !ctx.isYield() ) {
    }

    ctx.tryYield();

    if( common::isMoreTimeSinceTimestampThanInSec( m_lastDeviceStatusCheckingTime, AllDevicesReachableCheckTimeout ) ) {
        checkReachableDevicesState();
    }

    return ClbkRet::Continue;
}

size_t Gateway::getRecommendedThreadStackSize() const {
    return 9 * 1024;
}

bool Gateway::initialize() {
    hard::ResetZW::get().initialize();

    auto& znetInst = ZNet::get();
    while( !znetInst.isZnetInitCompleted() ) {
        znetInst.onLoop();
        // $HACK: This sleep was added to fix watchdog issue during startup.
        core::this_thread::sleep_for( 400ms );
    }

    return true;
}

void Gateway::startup( ScriptExecutor scriptExecutor ) {
    ZNet::get().startup( scriptExecutor );
}

bool Gateway::checkReachableDevicesState() {
    m_lastDeviceStatusCheckingTime = common::getTimeSinceBootInSec();
    return true;
}

IGateway::AddingDeviceStatus Gateway::restoreDeviceInfo( const DeviceId& deviceId, storage::File& file ) {
    AddingDeviceStatus addingResult = AddingDeviceStatus::OK;
    auto device = DevicesStorage::get().restoreDevice( deviceId, file, addingResult );
    if( device ) {
        m_devices.emplace_back( device );
    }
    return addingResult;
}

void Gateway::cleanAllData() {
    std::vector<znet_node_id_t> node_ids;
    for( auto it : m_devices ) {
        const auto& deviceId = ( *it ).getId();
        node_ids.push_back( deviceId );
    }
    for( znet_node_id_t node_id : node_ids ) {
        ZNet::get().nodeRemove( node_id );
    }

    m_devices.clear();
}

Gateway::DevicePtr Gateway::addDevice( uint8_t deviceId ) {
    auto itFind = std::find_if( m_devices.begin(), m_devices.end(), [&deviceId]( const DevicePtr& device ) {
        return device->getId() == deviceId;
    } );

    if( itFind != m_devices.end() ) {
        return *itFind;
    }

    auto device = std::make_shared<devices::Device>( deviceId,
                                                     &gateways::zwave::ZNet::get().getRequestsManager(),
                                                     &gateways::zwave::ZNet::get(),
                                                     &gateways::zwave::ZNet::get().getReachableCheckTimeoutSettings() );

    m_devices.reserve( m_devices.size() + 1 );
    m_devices.emplace_back( device );

    return device;
}

bool Gateway::removeDevice( uint8_t deviceId ) {
    for( auto it = m_devices.begin(), itEnd = m_devices.end(); it != itEnd; ++it ) {
        if( ( *it )->getId() == deviceId ) {
            DevicesStorage::get().removeDeviceFile( deviceId );
            m_devices.erase( it );
            DeviceSettingsManager::get().onDeviceRemoved( deviceId, GatewayID::ZWave );
            return true;
        }
    }

    return false;
}

Gateway::DevicePtr Gateway::getDevice( uint8_t deviceId ) {
    auto itFind = std::find_if( m_devices.begin(), m_devices.end(), [&deviceId]( const DevicePtr& device ) {
        return device->getId() == deviceId;
    } );

    return itFind == m_devices.end() ? Gateway::DevicePtr() : *itFind;
}

Gateway::DevicePtr Gateway::getDeviceByIndex( size_t idx ) {
    size_t currentIdx = 0;
    for( auto it = m_devices.begin(), itEnd = m_devices.end(); it != itEnd; ++it, ++currentIdx ) {
        if( currentIdx == idx ) {
            return *it;
        }
    }

    return DevicePtr();
}

size_t Gateway::getDevicesCount() {
    return m_devices.size();
}

}  // namespace zwave
}  // namespace gateways
