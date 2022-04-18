#pragma once

#include <znet/znet_provisioning_list_entry.hpp>

#include "BusCommand.h"

namespace gateways::commands {

class ProvisioningListEntryUpdated : public ::bus::IBusCommand {
public:
    ProvisioningListEntryUpdated( uint32_t entryId, ::znet::ProvisioningListEntryFields fieldDiff )
    : m_entryId( entryId )
    , m_fieldDiff( std::move( fieldDiff ) ) {}

    auto getEntryId() const {
        return m_entryId;
    }

    auto getFieldDiff() const {
        return m_fieldDiff;
    }

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    uint32_t m_entryId;
    ::znet::ProvisioningListEntryFields m_fieldDiff;
};

}  // namespace gateways::commands
