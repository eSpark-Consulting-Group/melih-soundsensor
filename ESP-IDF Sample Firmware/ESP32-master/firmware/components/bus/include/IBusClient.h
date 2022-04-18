#pragma once

#include "sdkconfig.h"

#include "IInitialize.h"
#include "BusCommand.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace bus {

class IBusClient : public IBusCommandVisitor {
    friend class Bus;

public:
#ifdef CONFIG_SPIRAM
    static constexpr auto DEFAULT_MSG_QUEUE_SIZE = 64;
#else   // not CONFIG_SPIRAM
    static constexpr auto DEFAULT_MSG_QUEUE_SIZE = 32;
#endif  // CONFIG_SPIRAM

    IBusClient( const unsigned int msgLen = DEFAULT_MSG_QUEUE_SIZE );

    virtual ~IBusClient();

    void leaveBus();

    void send( const BusAddr to, std::unique_ptr<IBusCommand> msg ) const;

    void send( const char* to, std::unique_ptr<IBusCommand> msg ) const;

    template <typename CmdType, typename AddrType, typename... Args>
    inline void send( const AddrType to, Args&&... args ) const {
        auto cmd = std::make_unique<CmdType>( std::forward<Args>( args )... );
        send( to, std::move( cmd ) );
    }

    /**
     * Same as waitToResp except that it returns std::future
     */
    template <typename CmdType, typename AddrType, typename... Args>
    inline auto asyncSend( const AddrType to, Args&&... args ) const {
        auto cmd = std::make_unique<CmdType>( std::forward<Args>( args )... );
        auto future = cmd->getFuture();
        send( to, std::move( cmd ) );
        return future;
    }

    template <typename CmdType, typename AddrType, typename DelayType, typename... Args>
    auto waitToResp( const AddrType to, const DelayType timeout, Args&&... args ) const {
        auto result = asyncSend<CmdType>( to, std::forward<Args>( args )... );
        auto statusOfResult = result.wait_for( timeout );
        if( statusOfResult != std::future_status::ready ) {
            return CmdType::createExecTimeoutErrorResult();
        }
        return result.get();
    }

    template <typename CmdType, typename AddrType, typename... Args>
    auto waitResp( const AddrType to, Args&&... args ) const {
        auto result = asyncSend<CmdType>( to, std::forward<Args>( args )... );
        result.wait();
        return result.get();
    }

    inline BusAddr addr() const {
        return m_addr;
    }

    inline auto queue() {
        return m_queue;
    }

    static void sendToClients( BusAddr to, std::unique_ptr<BusMsg> busMsg );

protected:
    void tryReceive( TickType_t waitTime );

    bool tryReceiveSingleMessage( TickType_t waitTime );

    bool tryReceiveSingleMessage() {
        constexpr auto delaySysTics = 0;
        return tryReceiveSingleMessage( delaySysTics );
    }

    virtual void receive( const BusAddr from, const BusAddr to, IBusCommand& msg ) {
        msg.accept( from, to, *this );
    }

private:
    static BusAddr m_addrLast;
    const BusAddr m_addr = ( ++m_addrLast );
    QueueHandle_t m_queue = nullptr;
};

}  // namespace bus
