#include <cinttypes>
#include <cstring>

#include "ProvisioningListStorage.h"
#include "ProvisioningListEntrySerializer.h"
#include "KeyValueStorageFactory.h"
#include "DebugUtils.h"

static constexpr auto STORAGE_NAME_PRONISIONING_LIST = "zwave_provisioning_list";

namespace gateways {
namespace zwave {

ProvisioningListStorage::ProvisioningListStorage( std::string storageName,
                                                  storage::KeyValueStorageFactory& storageFactory )
: m_storageName( std::move( storageName ) )
, m_storageFactory( storageFactory ) {}

bool ProvisioningListStorage::storeProvisioningListEntry( const ::znet::ProvisioningListEntry& entry ) {
    auto store = m_storageFactory.openNamespace( m_storageName );
    if( !store ) {
        return false;
    }

    char key[16];
    snprintf( key, sizeof( key ), "%" PRIX32, entry.m_provisioningId );
    ProvisioningListEntrySerializer serializer;
    std::vector<char> vec = serializer.serialize( entry );
    if( !store->setBlob( key, vec ) ) {
        FUNC_LOGE( "Failed to store data with key: %d", entry.m_provisioningId );
        return false;
    }
    return true;
}

bool ProvisioningListStorage::removeProvisioningListEntryByID( uint32_t id ) {
    auto store = m_storageFactory.openNamespace( m_storageName );
    if( !store ) {
        return false;
    }

    char key[16];
    snprintf( key, sizeof( key ), "%" PRIX32, id );

    if( !store->remove( key ) ) {
        FUNC_LOGE( "Failed to store data with key: %d", id );
        return false;
    }
    return true;
}

bool ProvisioningListStorage::findProvisioningListEntryByID( uint32_t entryId,
                                                             ::znet::ProvisioningListEntry& entry ) const {
    auto store = m_storageFactory.openNamespace( m_storageName );
    if( !store ) {
        return false;
    }

    char key[16];
    snprintf( key, sizeof( key ), "%" PRIX32, entryId );

    std::vector<char> blobData;
    if( !store->getBlob( key, blobData ) ) {
        return false;
    }
    ::znet::ProvisioningListEntry deserializedEntry;
    if( !ProvisioningListEntrySerializer{}.deserialize( std::move( blobData ), deserializedEntry ) ) {
        return false;
    }
    entry = deserializedEntry;
    return true;
}

bool ProvisioningListStorage::findProvisioningListEntryByDSK( const ::znet::DSK& dsk,
                                                              ::znet::ProvisioningListEntry& entry ) const {
    auto store = m_storageFactory.openNamespace( m_storageName );
    if( !store ) {
        return false;
    }

    auto end = store->cend();
    for( auto itr = store->cbegin(); itr != end; ++itr ) {
        auto record = itr.value();
        if( !record.is_type<storage::KeyValueStorage::BlobType>() ) {
            FUNC_LOGE( "Value type isn't BlobType!" );
            continue;
        }

        ProvisioningListEntrySerializer serializer;
        if( !serializer.deserialize( std::move( record.get<storage::KeyValueStorage::BlobType>() ), entry ) ) {
            continue;
        }

        if( 0 == std::memcmp( entry.m_dsk.data(), dsk.data(), dsk.size() * sizeof( dsk[0] ) ) ) {
            return true;
        }
    }

    return false;
}

bool ProvisioningListStorage::loadProvisioningList(
    const IProvisioningListProvider::ProcessProvisioningCallback& clbk ) const {
    auto store = m_storageFactory.openNamespace( m_storageName );
    if( !store ) {
        return false;
    }
    auto end = store->cend();
    for( auto itr = store->cbegin(); end != itr; ++itr ) {
        ::znet::ProvisioningListEntry entry;
        auto record = itr.value();

        ProvisioningListEntrySerializer serializer;
        if( !record.is_type<storage::KeyValueStorage::BlobType>() ||
            record.get<storage::KeyValueStorage::BlobType>().empty() ||
            !serializer.deserialize( std::move( record.get<storage::KeyValueStorage::BlobType>() ), entry ) ) {
            return false;
        }

        if( clbk( entry ) ) {
            return true;
        }
    }
    return true;
}

size_t ProvisioningListStorage::getProvisioningListEntryCount() const {
    size_t size = 0;

    auto store = m_storageFactory.openNamespace( m_storageName );
    if( !store ) {
        return 0;
    }

    auto end = store->cend();
    for( auto itr = store->cbegin(); end != itr; ++itr ) {
        ::znet::ProvisioningListEntry entry;
        auto record = itr.value();
        ProvisioningListEntrySerializer serializer;
        if( !record.is_type<storage::KeyValueStorage::BlobType>() ||
            record.get<storage::KeyValueStorage::BlobType>().empty() ||
            !serializer.deserialize( std::move( record.get<storage::KeyValueStorage::BlobType>() ), entry ) ) {
            continue;
        }

        size++;
    }

    return size;
}

}  // namespace zwave
}  // namespace gateways
