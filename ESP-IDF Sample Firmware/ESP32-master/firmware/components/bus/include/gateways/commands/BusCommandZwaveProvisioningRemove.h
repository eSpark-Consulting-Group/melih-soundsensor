#pragma once

#include <unordered_set>

#include <znet/iznet_provisioning_list_controller.hpp>

#include "BusCommand.h"

namespace gateways::commands {

class ZwaveProvisioningRemove final
: public bus::BusCommandWithDefaultInitializedResponse<::znet::api::IProvisioningListController::OperationStatus> {
public:
    explicit ZwaveProvisioningRemove( std::unordered_set<uint32_t> provisioningIdList )
    : m_provisioningIdList( std::move( provisioningIdList ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const auto& getProvisioningIdList() const {
        return m_provisioningIdList;
    }

private:
    std::unordered_set<uint32_t> m_provisioningIdList;
};

}  // namespace gateways::commands
