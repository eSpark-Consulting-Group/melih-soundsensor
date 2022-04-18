#pragma once
#include "KeyValueStorage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <thread>
#include <memory>
#include <IReadStream.h>
namespace storage {

class NVS_Initializer {
public:
    static NVS_Initializer& get() {
        static NVS_Initializer initializer;
        return initializer;
    }
    bool init();
    bool deinit();
    bool isInitialized();

private:
    NVS_Initializer( const NVS_Initializer& ) = delete;
    NVS_Initializer& operator=( const NVS_Initializer& ) = delete;
    NVS_Initializer()
    : m_isInitialized( false ) {}
    bool m_isInitialized;
};

// TODO: implement as singleton with simple thread-safe mechanism. Need be research and improve.
class SystemStorage : public KeyValueStorage {
private:
    nvs_handle_t m_NvsHandle;
    bool commit();
    std::string m_NvsNamespace;  // TODO: implement api to support different namespaces
    SystemStorage( const SystemStorage& ) = delete;
    SystemStorage& operator=( const SystemStorage& ) = delete;
    SystemStorage();

public:
    static SystemStorage& get();
    bool isOpened();
    virtual ~SystemStorage();

    virtual bool getInt8( const std::string& key, std::int8_t& value );
    virtual bool getUInt8( const std::string& key, std::uint8_t& value );
    virtual bool getInt16( const std::string& key, std::int16_t& value );
    virtual bool getUInt16( const std::string& key, std::uint16_t& value );
    virtual bool getInt32( const std::string& key, std::int32_t& value );
    virtual bool getUInt32( const std::string& key, std::uint32_t& value );
    virtual bool getInt64( const std::string& key, std::int64_t& value );
    virtual bool getBlob( const std::string& key, BlobType& value );
    virtual bool setInt8( const std::string& key, std::int8_t value );
    virtual bool setUInt8( const std::string& key, std::uint8_t value );
    virtual bool setInt16( const std::string& key, std::int16_t value );
    virtual bool setUInt16( const std::string& key, std::uint16_t value );
    virtual bool setInt32( const std::string& key, std::int32_t value );
    virtual bool setUInt32( const std::string& key, std::uint32_t value );
    virtual bool setInt64( const std::string& key, std::int64_t value );
    virtual bool setBlob( const std::string& key, const BlobType& value );
    virtual bool setBlob( const std::string& key, common::IReadStream& rstream, const size_t len );
    virtual bool setStr( const std::string& key, const std::string& str );
    virtual bool getStr( const std::string& key, std::string& str );

    virtual bool exists( const std::string& key );
    virtual bool remove( const std::string& key );
    virtual bool removeAll();
};

}  // namespace storage
