#include "Bus.h"

#include <algorithm>
#include <functional>

#include "freertos/task.h"
#include <esp_debug_helpers.h>

#include "DebugUtils.h"

namespace bus {

Bus& Bus::get() {
    static Bus s_bus;
    return s_bus;
}

void Bus::connect( IBusClient* client ) {
    std::lock_guard<decltype( m_mutex )> lock( m_mutex );

    m_addrToClient[client->addr()] = client;
}

void Bus::disconnect( IBusClient* client ) {
    std::lock_guard<decltype( m_mutex )> lock( m_mutex );

    m_addrToClient.erase( client->addr() );

    for( auto& pair : m_addrToGroup ) {
        pair.second.erase( client );
    }
}

bool Bus::join( BusAddr addr, IBusClient& client ) {
    std::lock_guard<decltype( m_mutex )> lock( m_mutex );

    auto& group = m_addrToGroup[addr];
    group.emplace( &client );
    return true;
}

bool Bus::join( const char* addrName, IBusClient& client ) {
    if( !addrName ) {
        return false;
    }

    BusAddr addr = std::hash<const char*>{}( addrName );
    return join( addr, client );
}

void Bus::leave( BusAddr addr, IBusClient& client ) {
    std::lock_guard<decltype( m_mutex )> lock( m_mutex );

    auto& group = m_addrToGroup[addr];
    group.erase( &client );
}

void Bus::leave( const char* addrName, IBusClient& client ) {
    if( !addrName ) {
        return;
    }

    BusAddr addr = std::hash<const char*>{}( addrName );
    return leave( addr, client );
}

const Bus::AddrList& Bus::getClientsByGroupAddr( const BusAddr addr ) const {
    static AddrList emptyList;

    auto itr = m_addrToGroup.find( addr );
    if( m_addrToGroup.end() != itr ) {
        return itr->second;
    }

    return emptyList;
}

IBusClient* Bus::getClientByAddr( const BusAddr addr ) const {
    auto itr = m_addrToClient.find( addr );
    if( m_addrToClient.end() != itr ) {
        return itr->second;
    }

    return nullptr;
}

BusAddr Bus::nameToAddr( const char* addrName ) {
    if( !addrName ) {
        return BusAddrInvalid;
    }
    BusAddr addr = std::hash<const char*>{}( addrName );

    std::lock_guard<decltype( m_mutex )> lock( m_mutex );
    auto i = m_addrToGroup.find( addr );
    if( m_addrToGroup.end() != i ) {
        return i->first;
    }

    return BusAddrInvalid;
}

bool sendFromNowhere( const BusAddr to, std::unique_ptr<IBusCommand>&& msg ) {
    if( BusAddrInvalid == to || !msg ) {
        constexpr auto depth = 100;
        esp_backtrace_print( depth );
        return false;
    }

    msg->setFromTo( BusAddrInvalid, to );
    IBusClient::sendToClients( to, std::move( msg ) );
    return true;
}

bool sendFromNowhere( const char* to, std::unique_ptr<IBusCommand>&& msg ) {
    msg->isGroup = true;
    return sendFromNowhere( Bus::get().nameToAddr( to ), std::move( msg ) );
}

}  // namespace bus
