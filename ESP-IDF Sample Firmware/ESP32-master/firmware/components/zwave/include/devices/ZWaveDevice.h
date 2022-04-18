#pragma once

#include <GatewayDefs.h>
#include <stdint.h>
#include <znet/znet_node.hpp>

namespace gateways {
namespace zwave {
namespace devices {

class Device : public znet::ZNode {
public:
    Device( uint8_t nodeId,
            znet::RequestsManager* requestManager,
            znet::IAssociationsConfigurator* associationsConfigurator,
            znet::ReachableCheckTimeoutSettings* reachableCheckTimeoutSettings );

public:
    void onDataUpdated();

    void resetDataUpdateTimestamp();

    bool needToUpdate() const;

private:
    uint32_t m_lastDataUpdateTime = 0;
};

}  // namespace devices
}  // namespace zwave
}  // namespace gateways
