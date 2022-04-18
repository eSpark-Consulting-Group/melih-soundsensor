#pragma once

#include <unordered_set>

#include <znet/iznet_provisioning_list_controller.hpp>
#include <utility>

#include "BusCommand.h"

namespace gateways::commands {

class ZwaveProvisioningList
: public bus::BusCommandWithDefaultInitializedResponse<::znet::api::IProvisioningListController::OperationStatus> {
public:
    explicit ZwaveProvisioningList( std::function<bool( const ::znet::ProvisioningListEntry& )> processor,
                                    std::unordered_set<uint8_t> nodeIdList,
                                    std::unordered_set<uint32_t> provisioningIdList )
    : m_processor( std::move( processor ) )
    , m_nodeIdList( std::move( nodeIdList ) )
    , m_provisioningIdList( std::move( provisioningIdList ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const auto& getNodeIdList() const {
        return m_nodeIdList;
    }

    const auto& getProvisioningIdList() const {
        return m_provisioningIdList;
    }

    auto getProcessor() const {
        return m_processor;
    }

private:
    // Return value indicates whether we should stop enumeration: true - interrupt, false - continue.
    std::function<bool( const ::znet::ProvisioningListEntry& )> m_processor;

    std::unordered_set<uint8_t> m_nodeIdList;
    std::unordered_set<uint32_t> m_provisioningIdList;
};

}  // namespace gateways::commands
