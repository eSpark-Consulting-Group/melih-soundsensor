#include "BinaryFileStorageFactory.h"
#include "BinaryFileStorage.h"
#include "Storage.h"
#include <dirent.h>
#include <sys/stat.h>

#include "DebugUtils.h"

#define KEEP_STOR_IN_MEM

namespace storage {

BinaryFileStorageFactory& BinaryFileStorageFactory::get() {
    static BinaryFileStorageFactory self;
    return self;
}

bool BinaryFileStorageFactory::checkAndCreateStorageDirectory() {
    return Storage::get().createDir( STORAGE_PATH );
}

BinaryFileStorageFactory::BinaryFileStorageFactory() {
    checkAndCreateStorageDirectory();
}

KeyValueStorage* BinaryFileStorageFactory::create( const std::string& _namespace ) {
    std::lock_guard<std::mutex> guard( m_mutex );

    StringToStorageMap::iterator lb = m_namespace_to_storage.lower_bound( _namespace );

    if( lb != m_namespace_to_storage.end() && !( m_namespace_to_storage.key_comp()( _namespace, lb->first ) ) ) {
        ++( lb->second )->m_ref_counter;
        return lb->second;
    }

    BinaryFileStorage* st = new BinaryFileStorage( *this, _namespace );
    m_namespace_to_storage.insert( lb, StringToStorageMap::value_type( _namespace, st ) );
    return st;
}

void BinaryFileStorageFactory::detach( KeyValueStorage* _storage ) {
    BinaryFileStorage* st = static_cast<BinaryFileStorage*>( _storage );
    std::lock_guard<std::mutex> guard( m_mutex );
    --st->m_ref_counter;
#ifndef KEEP_STOR_IN_MEM
    if( 0 == st->m_ref_counter ) {
        m_namespace_to_storage.erase( st->m_namespace );
        delete _storage;
    }
#endif
}

bool BinaryFileStorageFactory::isDBExist( const std::string& _namespace ) const {
    auto fullPath = std::string( BinaryFileStorageFactory::STORAGE_PATH ) + "/" + _namespace;
    return storage::Storage::get().isExist( fullPath );
}

bool BinaryFileStorageFactory::removeDBFile( const std::string& _namespace ) {
    auto fullPath = std::string( BinaryFileStorageFactory::STORAGE_PATH ) + "/" + _namespace;
    return storage::Storage::get().remove( fullPath );
}

void BinaryFileStorageFactory::reset() {
    std::lock_guard<std::mutex> guard( m_mutex );

    auto end = m_namespace_to_storage.end();
    for( auto itr = m_namespace_to_storage.begin(); itr != end; ++itr ) {
        if( 0 < itr->second->m_ref_counter ) {
            FUNC_LOGE( "BinaryFileStorage instance for namespace=%s didn't closed before reset!",
                       itr->second->m_namespace.c_str() );
        }
        delete itr->second;
    }
    m_namespace_to_storage.clear();
}

}  // namespace storage
