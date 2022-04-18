#pragma once

#include <znet/znet_provisioning_list_entry.hpp>

#include "BusCommand.h"

namespace gateways::commands {

class ProvisioningListEntryRemoved : public ::bus::IBusCommand {
public:
    explicit ProvisioningListEntryRemoved( uint32_t entryId )
    : m_entryId( entryId ) {}

    uint32_t getEntryId() const {
        return m_entryId;
    }

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    uint32_t m_entryId;
};

}  // namespace gateways::commands
