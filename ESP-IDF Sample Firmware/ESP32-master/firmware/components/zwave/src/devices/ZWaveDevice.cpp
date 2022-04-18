
#include <devices/ZWaveDevice.h>
#include <ZWaveDefs.h>
#include <Utils.h>

namespace gateways {
namespace zwave {
namespace devices {

Device::Device( uint8_t nodeId,
                znet::RequestsManager* requestManager,
                znet::IAssociationsConfigurator* associationsConfigurator,
                znet::ReachableCheckTimeoutSettings* reachableCheckTimeoutSettings )
: znet::ZNode( requestManager, associationsConfigurator, reachableCheckTimeoutSettings ) {
    init( nodeId, ::znet::SecurityMode::Class::None );
}

void Device::onDataUpdated() {
    m_lastDataUpdateTime = common::getTimeSinceBootInSec();
}

void Device::resetDataUpdateTimestamp() {
    m_lastDataUpdateTime = 0;
}

bool Device::needToUpdate() const {
    return ( m_lastDataUpdateTime == 0 ) ||
           common::isMoreTimeSinceTimestampThanInSec( m_lastDataUpdateTime, EachDeviceReachableCheckTimeout );
}

}  // namespace devices
}  // namespace zwave
}  // namespace gateways
