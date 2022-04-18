#pragma once

#include "KeyValueStorage.h"
#include <memory>
#include <functional>

namespace storage {

class KeyValueStorageFactory {
    struct KeyValueStorageDeleter {
        KeyValueStorageFactory* m_owner = nullptr;
        void operator()( KeyValueStorage* _storage ) {
            m_owner->detach( _storage );
        }
    };

public:
    typedef std::unique_ptr<KeyValueStorage, KeyValueStorageDeleter> StoragePtr;
    virtual ~KeyValueStorageFactory() = default;
    StoragePtr openNamespace( const std::string& _namespace ) {
        const std::string& ns = _namespace.empty() ? "DEFAULT" : _namespace;  // TODO  remove  DEFAULT
        return StoragePtr( create( ns ), KeyValueStorageDeleter{ this } );
    }

    virtual bool isDBExist( const std::string& _namespace ) const {
        return false;
    }

    virtual bool removeDBFile( const std::string& _namespace ) {
        return false;
    }

    virtual void reset() {
        // Nothing to do
    }

protected:
    virtual KeyValueStorage* create( const std::string& _namespace ) = 0;
    virtual void detach( KeyValueStorage* _storage ) = 0;
};

}  // namespace storage
