#pragma once

#include <znet/iznet_provisioning_list_provider.hpp>

namespace storage {
class KeyValueStorageFactory;
}

namespace gateways::zwave {

class ProvisioningListStorage : public ::znet::IProvisioningListProvider {
public:
    explicit ProvisioningListStorage( std::string storageName, ::storage::KeyValueStorageFactory& storageFactory );

    bool storeProvisioningListEntry( const znet::ProvisioningListEntry& ) override;
    bool removeProvisioningListEntryByID( uint32_t ) override;
    bool findProvisioningListEntryByID( uint32_t, znet::ProvisioningListEntry& ) const override;
    bool findProvisioningListEntryByDSK( const znet::DSK&, znet::ProvisioningListEntry& ) const override;
    bool loadProvisioningList( const znet::IProvisioningListProvider::ProcessProvisioningCallback& ) const override;
    size_t getProvisioningListEntryCount() const override;

private:
    const std::string m_storageName;
    ::storage::KeyValueStorageFactory& m_storageFactory;
};

}  // namespace gateways::zwave
