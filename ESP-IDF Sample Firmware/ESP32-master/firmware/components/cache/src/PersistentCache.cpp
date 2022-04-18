#include <PersistentCache.h>
#include <StorageSection.h>
#include <BinaryFileStorageFactory.h>
#include <IValue.h>
#include <IValueVisitor.h>
#include "CommonResources.h"
#include "IReadStream.h"

#include "lua/responce/HubSendBroadcast.h"

//#define ENABLE_PCH_LOGS
#ifdef ENABLE_PCH_LOGS

#include <DebugUtils.h>

#define PCH_LOG_TAG "   -----/\\/\\/\\/\\ [PersistentCache] "
#define PCH_LOGI( _FMT, ... ) ESP_LOGI( PCH_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define PCH_LOGE( _FMT, ... ) ESP_LOGE( PCH_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define PCH_LOGW( _FMT, ... ) ESP_LOGW( PCH_LOG_TAG, _FMT, ##__VA_ARGS__ )

#else

#define PCH_LOGI( ... ) \
    do {                \
    } while( false )
#define PCH_LOGE( ... ) \
    do {                \
    } while( false )
#define PCH_LOGW( ... ) \
    do {                \
    } while( false )

#endif

namespace cache {

using namespace std::chrono_literals;

namespace {

using BlobType = storage::KeyValueStorage::BlobType;

PersistentCache* g_instance = nullptr;

common::IValue::UPtr toIValue( storage::KeyValueStorage::ValueT& value ) {
    if( !value.is_type<BlobType>() ) {
        PCH_LOGE( "Value type isn't BlobType!" );
        return nullptr;
    }
    auto blob = value.get<BlobType>();
    common::ArrayRStream stream( blob.data(), blob.size() );

    return common::IValue::create( stream );
}

}  // namespace

void PersistentCache::ValueData::update( ValueT&& value ) {
    m_value = std::move( value );
    if( isExpired() ) {
        m_state = Added;
    }
    else {
        m_state = Updated;
    }
}

void PersistentCache::ValueData::remove() {
    m_value.release();
    m_state = Removed;
}

bool PersistentCache::ValueData::shouldSave( const TimePointT now ) const {
    if( Added == m_state ) {
        return true;
    }
    if( Updated != m_state ) {
        return false;
    }
    auto period = std::chrono::duration_cast<std::chrono::minutes>( now - m_time ).count();
    return period >= PERIOD_MINUTES;
}

bool PersistentCache::ValueData::store( storage::KeyValueStorage& stor, const std::string& key ) {
    if( !stor.set( key, m_value ) ) {
        PCH_LOGE( "Can't write IValue to storage!" );
        return false;
    }

    m_state = Saved;
    m_time = ClockT::now();
    return true;
}

PersistentCache& PersistentCache::get() {
    return *g_instance;
}

PersistentCache::PersistentCache()
: core::Shutdownable( 5s ) {  // Let's shutdown cache after 4 seconds of shutdown was initiated
    PCH_LOGI( "Created instance" );
}

PersistentCache::~PersistentCache() {
    flush();

    PCH_LOGI( "Destroyed instance" );
}

void PersistentCache::init() {
    auto inst = std::make_unique<PersistentCache>();
    if( inst && inst->initialize() ) {
        g_instance = inst.get();
        core::upd_srv::add( std::move( inst ), "PersistentCache" );
    }
}

void PersistentCache::setValueByKey( const KeyT key, ValueT&& value ) {
    if( key.empty() || !value ) {
        return;
    }

    std::lock_guard<std::mutex> guard( m_mutex );
    PCH_LOGI( "setValueByKey key=%s %s", key.c_str(), value->toJson( "value" ).c_str() );

    auto iter = m_values.find( key );
    if( iter != m_values.end() ) {
        ( *iter ).second.update( std::move( value ) );
    }
    else {
        m_values.emplace( key, std::move( value ) );
    }
}

PersistentCache::ValueT PersistentCache::getValueByKey( const KeyT& key ) const {
    std::lock_guard<std::mutex> guard( m_mutex );
    auto itr = m_values.find( key );
    if( itr == m_values.end() ) {
        return nullptr;
    }

    if( itr->second.isRemoved() ) {
        return nullptr;
    }

    PCH_LOGI( "getValueByKey key=%s %s", key.c_str(), itr->second.value()->toJson( "value" ).c_str() );

    return itr->second.value()->clone();
}

void PersistentCache::remove( const KeyT& key ) {
    std::lock_guard<std::mutex> guard( m_mutex );
    PCH_LOGI( "Remove key=%s from cache", key.c_str() );

    auto itr = m_values.find( key );
    if( itr == m_values.end() ) {
        return;
    }

    itr->second.remove();
}

void PersistentCache::setExpired( const KeyT& key ) {
    std::lock_guard<std::mutex> guard( m_mutex );
    PCH_LOGI( "Set expired value for key=%s", key.c_str() );

    auto itr = m_values.find( key );
    if( itr != m_values.end() ) {
        itr->second.setExpired();
    }
}

bool PersistentCache::isExpired( const KeyT& key ) const {
    std::lock_guard<std::mutex> guard( m_mutex );

    auto itr = m_values.find( key );
    if( itr != m_values.end() ) {
        return itr->second.isExpired();
    }
    return true;
}

bool PersistentCache::loadFromStorage() {
    std::lock_guard<std::mutex> guard( m_mutex );
    PCH_LOGI( "loadFromStorage..." );

    auto stor = getStor();
    if( !stor ) {
        return false;
    }

    std::list<std::string> brokenValues;
    auto end = stor->cend();
    for( auto itr = stor->cbegin(); itr != end; ++itr ) {
        auto value = itr.value();
        auto ival = toIValue( value );
        if( ival ) {
            m_values.emplace( *itr, ValueData( std::move( ival ), ValueData::Restored ) );
        }
        else {
            brokenValues.push_back( *itr );
        }
    }

    PCH_LOGI( "Loaded %i IValue's from storage", m_values.size() );

    // Remove broken values from storage
    int removedCount = 0;
    for( auto& key : brokenValues ) {
        stor->remove( key );
        ++removedCount;
    }
    if( removedCount ) {
        PCH_LOGI( "Removed %i broken IValue's from storage", removedCount );
    }

    return true;
}

void PersistentCache::flush() {
    std::lock_guard<std::mutex> guard( m_mutex );
    PCH_LOGI( "flush..." );

    auto stor = getStor();
    auto end = m_values.end();
    for( auto itr = m_values.begin(); itr != end; ++itr ) {
        if( !itr->second.isUpdated() ) {
            continue;
        }

        if( itr->second.store( *stor, itr->first ) ) {
            PCH_LOGI( "Saved to storage key=%s", itr->first.c_str() );
        }
        else {
            PCH_LOGE( "Failed to save to storage key=%s", itr->first.c_str() );
            break;
        }
    }
}

void PersistentCache::trySaveToStorage( UpdContext& ctx ) {
    auto now = ClockT::now();

    if( m_values.empty() ) {
        auto stor = getStor();
        if( stor ) {
            stor->removeAll();
        }
        return;
    }

    auto itr = m_values.begin();
    auto end = m_values.end();
    for( ; itr != end; ++itr ) {
        if( itr->second.shouldSave( now ) || itr->second.isRemoved() ) {
            break;
        }
    }
    if( itr == end ) {
        return;
    }

    auto stor = getStor();
    if( !stor ) {
        return;
    }

    while( itr != end ) {
        if( itr->second.isRemoved() ) {
            PCH_LOGI( "Trying to remove from storage key=%s", itr->first.c_str() );
            stor->remove( itr->first );
            itr = m_values.erase( itr );
            continue;
        }

        if( itr->second.shouldSave( now ) && itr->second.store( *stor, itr->first ) ) {
            PCH_LOGI( "Saved to storage key=%s", itr->first.c_str() );
        }

        ++itr;
        ctx.tryYield();
    }
}

PersistentCache::StorUPtr PersistentCache::getStor() const {
    return storage::BinaryFileStorageFactory::get().openNamespace( storage::section::VALUE_CACHE );
}

bool PersistentCache::initializeImpl() {
    auto& busInst = bus::Bus::get();
    return core::Shutdownable::initializeImpl() && loadFromStorage() && busInst.join( lua::responce::BUS_NAME, *this );
}

PersistentCache::ClbkRet PersistentCache::updateComponent( UpdContext& ctx ) {
    trySaveToStorage( ctx );
    return ClbkRet::Continue;
}

void PersistentCache::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::HubSendBroadcast& cmd ) {
    if( common::resources::broadcasts::events::IncludeFinished == cmd.m_event ) {
        flush();
    }
}

void PersistentCache::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::RemoveDevice& command ) {
    flush();
}

}  // namespace cache
