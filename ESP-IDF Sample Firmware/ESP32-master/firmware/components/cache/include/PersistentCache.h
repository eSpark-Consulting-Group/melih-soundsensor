#pragma once

#include <IValue.h>
#include <KeyValueStorageFactory.h>
#include "Bus.h"
#include "Shutdownable.h"

#include <chrono>
#include <mutex>
#include <unordered_map>

namespace cache {

class PersistentCache : public core::Shutdownable {
public:
    static constexpr auto PERIOD_MINUTES = 5;

    using ClockT = std::chrono::steady_clock;
    using TimePointT = ClockT::time_point;

    using KeyT = std::string;
    using ValueT = common::IValue::UPtr;

    using ClbkRet = core::upd_srv::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    struct ValueData {
        enum State { Added, Updated, Expired, Saved, Restored, Removed };

        ValueData( ValueT&& value, State state = Added, TimePointT time = ClockT::now() )
        : m_value( std::move( value ) )
        , m_state( state )
        , m_time( time ) {}

        void update( ValueT&& value );

        const ValueT& value() const {
            return m_value;
        }

        ValueT& value() {
            return m_value;
        }

        void remove();

        inline bool isUpdated() const {
            return Updated == m_state;
        }

        inline bool isRemoved() const {
            return Removed == m_state;
        }

        void setExpired() {
            if( Removed != m_state ) {
                m_state = Expired;
            }
        }

        inline bool isExpired() const {
            return Expired == m_state;
        }

        bool shouldSave( const TimePointT now ) const;

        bool store( storage::KeyValueStorage& stor, const std::string& key );

        ValueT m_value;
        State m_state;
        TimePointT m_time;
    };  // struct ValueData

    using MapT = std::unordered_map<KeyT, ValueData>;

    using StorUPtr = storage::KeyValueStorageFactory::StoragePtr;

    static PersistentCache& get();

    static void init();

    PersistentCache();

    ~PersistentCache();

    void setValueByKey( const KeyT key, ValueT&& value );

    ValueT getValueByKey( const KeyT& key ) const;

    void remove( const KeyT& key );

    void setExpired( const KeyT& key );

    bool isExpired( const KeyT& key ) const;

    bool loadFromStorage();

    void flush();

private:
    void trySaveToStorage( UpdContext& ctx );

    StorUPtr getStor() const;

private:
    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::HubSendBroadcast& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::RemoveDevice& command ) override;

private:
    mutable std::mutex m_mutex;
    MapT m_values;
};  // class PersistentCache

}  // namespace cache
