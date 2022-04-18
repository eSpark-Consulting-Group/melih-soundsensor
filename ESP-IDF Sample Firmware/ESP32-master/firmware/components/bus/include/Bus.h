#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <chrono>
#include <future>

#include "BusAddr.h"
#include "BusCommand.h"
#include "IBusClient.h"

namespace bus {

class Bus {
public:
    using AddrList = std::set<IBusClient*>;
    using AddrToGroup = std::map<BusAddr, AddrList>;
    using AddrToClient = std::map<BusAddr, IBusClient*>;

    static Bus& get();

    void connect( IBusClient* client );

    void disconnect( IBusClient* client );

    bool join( const char* addrName, IBusClient& client );

    void leave( const char* addrName, IBusClient& client );

    auto& mutex() {
        return m_mutex;
    }

    const AddrList& getClientsByGroupAddr( const BusAddr addr ) const;

    IBusClient* getClientByAddr( const BusAddr addr ) const;

    BusAddr nameToAddr( const char* addrName );

protected:
    bool join( BusAddr addr, IBusClient& client );

    void leave( BusAddr addr, IBusClient& client );

private:
    std::mutex m_mutex;
    AddrToGroup m_addrToGroup;
    AddrToClient m_addrToClient;
};

bool sendFromNowhere( const BusAddr to, std::unique_ptr<IBusCommand>&& msg );

bool sendFromNowhere( const char* to, std::unique_ptr<IBusCommand>&& msg );

template <typename CmdType, typename AddrType, typename... Args>
inline bool sendFromNowhere( const AddrType to, Args&&... args ) {
    auto cmd = std::make_unique<CmdType>( std::forward<Args>( args )... );
    return cmd && sendFromNowhere( to, std::move( cmd ) );
}

template <typename CmdType, typename AddrType, typename... Args>
inline auto sendFromNowhereAsync( const AddrType to, Args&&... args ) {
    auto cmd = std::make_unique<CmdType>( std::forward<Args>( args )... );
    auto future = cmd->getFuture();
    sendFromNowhere( to, std::move( cmd ) );
    return future;
}

template <typename CmdType, typename AddrType, typename DelayType, typename... Args>
auto waitToResp( const AddrType to, const DelayType timeout, Args&&... args ) {
    auto cmd = std::make_unique<CmdType>( std::forward<Args>( args )... );
    auto result = cmd->getFuture();
    if( !sendFromNowhere( to, std::move( cmd ) ) ) {
        return CmdType::createExecTimeoutErrorResult();
    }

    const auto statusOfResult = result.wait_for( timeout );
    return statusOfResult == std::future_status::ready ? result.get() : CmdType::createExecTimeoutErrorResult();
}

template <typename CmdType, typename AddrType, typename... Args>
auto waitResp( const AddrType to, Args&&... args ) {
    auto cmd = std::make_unique<CmdType>( std::forward<Args>( args )... );
    auto result = cmd->getFuture();
    if( sendFromNowhere( to, std::move( cmd ) ) ) {
        result.wait();
    }
    return result.get();
}

}  // namespace bus
