#include <DebugUtils.h>
#include "../include/MemCache.h"

namespace cache {

MemCache& MemCache::get() {
    static MemCache cache;
    return cache;
}

void MemCache::setValueByKey( const std::string& key, common::IValue::UPtr&& value ) {
    std::lock_guard<std::mutex> guard( m_mutex );

    if( key.empty() || !value ) {
        return;
    }

    auto iter = m_values.find( key );
    if( iter != m_values.end() ) {
        ( *iter ).second = std::move( value );
    }
    else {
        m_values.insert( std::make_pair( key, std::move( value ) ) );
    }
}

MemCache::ValueT MemCache::getValueByKey( const MemCache::KeyT& key ) {
    std::lock_guard<std::mutex> guard( m_mutex );

    if( m_values.find( key ) != m_values.end() ) {
        if( m_values[key] ) {
            return m_values[key]->clone();
        }
    }

    return nullptr;
}

void MemCache::remove( const MemCache::KeyT& key ) {
    std::lock_guard<std::mutex> guard( m_mutex );
    m_values.erase( key );
}

}  // namespace cache
