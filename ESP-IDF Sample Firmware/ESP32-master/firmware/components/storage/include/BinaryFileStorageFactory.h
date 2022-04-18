#pragma once

#include "KeyValueStorageFactory.h"
#include <map>
#include <mutex>

namespace storage {

class BinaryFileStorage;

class BinaryFileStorageFactory final : public KeyValueStorageFactory {
    friend class LfsKeyValueStorage;

    typedef std::map<std::string, BinaryFileStorage*> StringToStorageMap;
    StringToStorageMap m_namespace_to_storage;
    std::mutex m_mutex;

public:
    constexpr static auto STORAGE_PATH = "/vol/common";

    static BinaryFileStorageFactory& get();

    static bool checkAndCreateStorageDirectory();

    bool isDBExist( const std::string& _namespace ) const override;

    bool removeDBFile( const std::string& _namespace ) override;

    void reset() override;

private:
    BinaryFileStorageFactory();

    KeyValueStorage* create( const std::string& _namespace ) override;
    void detach( KeyValueStorage* _storage ) override;
};

}  // end of namespace storage
