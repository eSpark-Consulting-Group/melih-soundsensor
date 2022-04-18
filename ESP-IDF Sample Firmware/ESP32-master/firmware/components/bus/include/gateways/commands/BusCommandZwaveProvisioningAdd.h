#pragma once

#include <vector>
#include <string>
#include <functional>

#include <znet/iznet_provisioning_list_controller.hpp>

#include "BusCommand.h"

namespace gateways::commands {

class ZwaveProvisioningAdd
: public bus::BusCommandWithDefaultInitializedResponse<::znet::api::IProvisioningListController::OperationStatus> {
public:
    ZwaveProvisioningAdd(::znet::DSK dsk, ::znet::ProvisioningListEntryFields fields )
    : m_dsk( dsk )
    , m_fields( std::move( fields ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    const ::znet::DSK& getDsk() const {
        return m_dsk;
    }

    const ::znet::ProvisioningListEntryFields& getFields() const {
        return m_fields;
    }

private:
    ::znet::DSK m_dsk;
    ::znet::ProvisioningListEntryFields m_fields;
};

}  // namespace gateways::commands
