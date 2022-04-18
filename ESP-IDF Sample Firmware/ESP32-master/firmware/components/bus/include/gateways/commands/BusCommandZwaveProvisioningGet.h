#pragma once

#include <znet/znet_provisioning_list_entry.hpp>

#include "BusCommand.h"

namespace gateways::commands {

struct ZwaveProvisioningGetResponse {
    std::string m_name;
    std::string m_room;
};

class ZwaveProvisioningGet : public ::bus::BusCommandWithDefaultInitializedResponse<ZwaveProvisioningGetResponse> {
public:
    explicit ZwaveProvisioningGet( uint8_t nodeId )
    : m_nodeId( nodeId ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    static ZwaveProvisioningGetResponse createExecTimeoutErrorResult() {
        return {};
    }

    uint8_t getNodeId() const {
        return m_nodeId;
    }

private:
    uint8_t m_nodeId;
};

}  // namespace gateways::commands
