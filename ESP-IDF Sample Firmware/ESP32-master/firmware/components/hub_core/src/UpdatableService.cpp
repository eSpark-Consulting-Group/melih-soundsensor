#include "UpdatableService.h"
#include "Thread.h"

#include <mutex>
#include <list>
#include <set>
#include <atomic>

#include "DebugUtils.h"

namespace core {
namespace upd_srv {

namespace {

using namespace std::chrono_literals;

Uuid g_nextUuid = 0;

std::list<IUpdatable::SPtr> g_updatables;

std::list<IUpdatable::SPtr> g_toAddUpdatables;
std::mutex g_toAddUpdatablesMutex;

std::set<Uuid> g_toRemove;
std::mutex g_toRemoveMutex;

std::atomic<unsigned int> g_activeCount = 0;

class ClbkUpdatable : public IUpdatable {
public:
    ClbkUpdatable( UpdClbk&& clbk )
    : m_clbk( std::move( clbk ) ) {}

    ClbkRet updateComponent( UpdContext& ctx ) override {
        return m_clbk();
    }

private:
    UpdClbk m_clbk;
};

}  // namespace

ClbkRet IUpdatable::update( UpdContext& ctx ) {
    if( UpdatableState::Delay == m_updateState ) {
        if( ClockType::now() - m_timepoint < m_delay ) {
            return ClbkRet::Continue;
        }

        m_updateState = UpdatableState::Update;
        if( m_delayClbk ) {
            m_delayClbk();
            m_delayClbk = nullptr;
        }
        return updateComponent( ctx );
    }
    else if( UpdatableState::Update == m_updateState ) {
        return updateComponent( ctx );
    }

    return ClbkRet::Completed;
}

bool UpdContext::isYield() const {
    return 100ms < ClockType::now() - tp;
}

void UpdContext::yield() {
    this_thread::yield();
    tp = ClockType::now();
}

void UpdContext::sleep_for( const Duration& delay ) {
    core::this_thread::sleep_for( delay );
    tp = ClockType::now();
}

Uuid add( UpdClbk&& clbk, StringType&& name ) {
    auto clbkUpd = std::make_unique<ClbkUpdatable>( std::move( clbk ) );
    return add( std::move( clbkUpd ), std::move( name ) );
}

Uuid add( IUpdatable::SPtr&& updatable, StringType&& name ) {
    const std::lock_guard<std::mutex> lock( g_toAddUpdatablesMutex );
    auto uuid = g_nextUuid++;

    updatable->setUpdUuid( uuid );
    if( updatable->updName().empty() && !name.empty() ) {
        updatable->setUpdName( std::move( name ) );
    }

    g_toAddUpdatables.emplace_back( std::move( updatable ) );

    return uuid;
}

void remove( const Uuid uuid ) {
    const std::lock_guard<std::mutex> lock( g_toRemoveMutex );
    g_toRemove.emplace( uuid );
}

void update( UpdContext& ctx ) {
    {
        const std::lock_guard<std::mutex> lock( g_toAddUpdatablesMutex );
        if( !g_toAddUpdatables.empty() ) {
            g_updatables.splice( g_updatables.end(), std::move( g_toAddUpdatables ) );
        }
    }

    decltype( g_toRemove ) toRemove;
    {
        const std::lock_guard<std::mutex> lock( g_toRemoveMutex );
        toRemove = std::move( g_toRemove );
    }

    const auto endItr = g_updatables.end();
    auto itr = g_updatables.begin();
    const auto toRemoveEndItr = toRemove.end();
    unsigned int activeCount{ 0 };
    while( endItr != itr ) {
        ctx.tryYield();

        if( !( *itr ) ) {
            FUNC_LOGE( "UpdatableService: Empty pointer inside updatable list!" );
            itr = g_updatables.erase( itr );
            continue;
        }
        if( toRemoveEndItr != toRemove.find( ( *itr )->updUuid() ) ) {
            FUNC_LOGI( "UpdatableService: Managed to remove %s", ( *itr )->updName().c_str() );
            itr = g_updatables.erase( itr );
        }
        else if( ClbkRet::Completed == ( *itr )->update( ctx ) ) {
            FUNC_LOGI( "UpdatableService: Completed %s", ( *itr )->updName().c_str() );
            itr = g_updatables.erase( itr );
        }
        else {
            ++itr;
        }

        ++activeCount;
    }
    g_activeCount = activeCount;
}

inline bool isEmpty() {
    return 0 == g_activeCount;
}

void loop() {
    UpdContext ctx;
    do {
        update( ctx );
        ctx.yield();
    } while( !isEmpty() );
}

}  // namespace upd_srv
}  // namespace core
