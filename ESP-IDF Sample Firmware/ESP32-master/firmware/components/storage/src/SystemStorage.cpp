#include "SystemStorage.h"
#include <vector>
#include "DebugUtils.h"
#include <cstring>
#include <algorithm>
// TODO: NOT THREAD-SAFE
namespace storage {

bool NVS_Initializer::init() {
    if( m_isInitialized ) {
        return true;
    }
    auto ret = nvs_flash_init();
    if( ret != ESP_OK ) {
        FUNC_LOGE( "FLash initialisation failed! Error code = %d", ret );
    }
    m_isInitialized = true;
    return m_isInitialized;
}

bool NVS_Initializer::isInitialized() {
    return m_isInitialized;
}

bool NVS_Initializer::deinit() {
    if( !m_isInitialized ) {
        FUNC_LOGE( "NVS deinitialization failed! NVS is not initialized" );
        return false;
    }
    auto ret = nvs_flash_deinit();
    if( ret != ESP_OK ) {
        FUNC_LOGE( "FLash initialization failed! Error code = %d", ret );
        return false;
    }
    m_isInitialized = false;
    return m_isInitialized;
}

SystemStorage& SystemStorage::get() {
    static SystemStorage stor;
    return stor;
}

SystemStorage::SystemStorage()
: m_NvsNamespace( "nvs" ) {  // TODO: implement api to support different namescpaces
    if( !storage::NVS_Initializer::get().init() ) {
        FUNC_LOGE( "Initialize of flash is failed" );
        // TODO: decide on actions in this case
    }
    if( nvs_open( "nvs", NVS_READWRITE, &m_NvsHandle ) != ESP_OK ) {
        FUNC_LOGE( "Opening of flash is failed" );
        // TODO: decide on actions in this case
    }
}

SystemStorage::~SystemStorage() {
    auto ret = storage::NVS_Initializer::get().deinit();
    if( !ret ) {
        FUNC_LOGE( "Deinitialize of flash is failed" );
        // TODO: decide on actions in this case
    }
    nvs_close( m_NvsHandle );
}

bool SystemStorage::commit() {
    return nvs_commit( m_NvsHandle ) == ESP_OK;
}

bool SystemStorage::getInt8( const std::string& key, std::int8_t& value ) {
    auto ret = nvs_get_i8( m_NvsHandle, key.c_str(), &value );
    return ret == ESP_OK;
};

bool SystemStorage::getUInt8( const std::string& key, uint8_t& value ) {
    auto ret = nvs_get_u8( m_NvsHandle, key.c_str(), &value );
    return ret == ESP_OK;
}

bool SystemStorage::getInt16( const std::string& key, std::int16_t& value ) {
    auto ret = nvs_get_i16( m_NvsHandle, key.c_str(), &value );
    return ret == ESP_OK;
};

bool SystemStorage::getUInt16( const std::string& key, uint16_t& value ) {
    auto ret = nvs_get_u16( m_NvsHandle, key.c_str(), &value );
    return ret == ESP_OK;
}

bool SystemStorage::getInt32( const std::string& key, std::int32_t& value ) {
    auto ret = nvs_get_i32( m_NvsHandle, key.c_str(), &value );
    return ret == ESP_OK;
};

bool SystemStorage::getUInt32( const std::string& key, uint32_t& value ) {
    auto ret = nvs_get_u32( m_NvsHandle, key.c_str(), &value );
    return ret == ESP_OK;
}

bool SystemStorage::getInt64( const std::string& key, std::int64_t& value ) {
    auto ret = nvs_get_i64( m_NvsHandle, key.c_str(), &value );
    return ret == ESP_OK;
};

bool SystemStorage::getBlob( const std::string& key, BlobType& value ) {
    size_t blobSize;
    if( nvs_get_blob( m_NvsHandle, key.c_str(), nullptr, &blobSize ) != ESP_OK ) {
        return false;
    }
    value.reserve( blobSize );
    auto ret = nvs_get_blob( m_NvsHandle, key.c_str(), reinterpret_cast<void*>( value.data() ), &blobSize );
    return ret == ESP_OK;
};

bool SystemStorage::setInt8( const std::string& key, std::int8_t value ) {
    auto ret = nvs_set_i8( m_NvsHandle, key.c_str(), value );
    return ret == ESP_OK && commit();
};

bool SystemStorage::setUInt8( const std::string& key, std::uint8_t value ) {
    auto ret = nvs_set_u8( m_NvsHandle, key.c_str(), value );
    return ret == ESP_OK && commit();
};

bool SystemStorage::setInt16( const std::string& key, std::int16_t value ) {
    auto ret = nvs_set_i16( m_NvsHandle, key.c_str(), value );
    return ret == ESP_OK && commit();
};

bool SystemStorage::setUInt16( const std::string& key, std::uint16_t value ) {
    auto ret = nvs_set_u16( m_NvsHandle, key.c_str(), value );
    return ret == ESP_OK && commit();
}

bool SystemStorage::setInt32( const std::string& key, std::int32_t value ) {
    auto ret = nvs_set_i32( m_NvsHandle, key.c_str(), value );
    return ret == ESP_OK && commit();
};

bool SystemStorage::setUInt32( const std::string& key, std::uint32_t value ) {
    auto ret = nvs_set_u32( m_NvsHandle, key.c_str(), value );
    return ret == ESP_OK && commit();
}

bool SystemStorage::setInt64( const std::string& key, std::int64_t value ) {
    auto ret = nvs_set_i64( m_NvsHandle, key.c_str(), value );
    return ret == ESP_OK && commit();
};

bool SystemStorage::setBlob( const std::string& key, const BlobType& value ) {
    auto ret = nvs_set_blob( m_NvsHandle, key.c_str(), reinterpret_cast<const void*>( value.data() ), value.size() );
    return ret == ESP_OK && commit();
};

bool SystemStorage::setBlob( const std::string& key, common::IReadStream& rstream, const size_t len ) {
    std::vector<char> buf;
    rstream.read( buf );
    return setBlob( key, buf );
};

bool SystemStorage::exists( const std::string& key ) {
    nvs_iterator_t it = nvs_entry_find( "nvs", m_NvsNamespace.c_str(), NVS_TYPE_ANY );
    while( it == nullptr ) {
        nvs_entry_info_t info;
        nvs_entry_info( it, &info );
        if( strcmp( key.c_str(), info.key ) == 0 ) {
            return true;
        }
        it = nvs_entry_next( it );
    }

    return false;
};

bool SystemStorage::remove( const std::string& key ) {
    auto ret = nvs_erase_key( m_NvsHandle, key.c_str() );
    return ret == ESP_OK && commit();
};

bool SystemStorage::removeAll() {
    FUNC_LOGE( "NOT IMPLEMENTED BY LEGACY REASON" );
    return false;
}

bool SystemStorage::setStr( const std::string& key, const std::string& str ) {
    auto ret = nvs_set_str( m_NvsHandle, key.c_str(), str.c_str() );
    return ret == ESP_OK && commit();
}

bool SystemStorage::getStr( const std::string& key, std::string& str ) {
    size_t strSize;

    if( nvs_get_str( m_NvsHandle, key.c_str(), nullptr, &strSize ) != ESP_OK ) {
        return false;
    }

    std::vector<char> tmpStr;
    tmpStr.reserve( strSize );
    auto ret = nvs_get_str( m_NvsHandle, key.c_str(), tmpStr.data(), &strSize );
    if( ret != ESP_OK ) {
        return false;
    }
    str = tmpStr.data();
    return commit();
}

bool SystemStorage::isOpened() {
    return false;
}

}  // namespace storage
