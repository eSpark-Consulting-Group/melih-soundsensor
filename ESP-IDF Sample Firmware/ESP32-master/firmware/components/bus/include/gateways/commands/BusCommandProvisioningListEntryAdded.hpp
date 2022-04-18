#pragma once

#include <znet/znet_provisioning_list_entry.hpp>

#include "BusCommand.h"

namespace gateways::commands {

class ProvisioningListEntryAdded : public ::bus::IBusCommand {
public:
    explicit ProvisioningListEntryAdded(::znet::ProvisioningListEntry entry )
    : m_entry( std::move( entry ) ) {}

    auto getEntry() const {
        return m_entry;
    }

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    ::znet::ProvisioningListEntry m_entry;
};

}  // namespace gateways::commands
