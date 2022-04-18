#include "IBusClient.h"
#include "Bus.h"

#include <esp_debug_helpers.h>

namespace bus {

BusAddr IBusClient::m_addrLast = 0;

IBusClient::IBusClient( const unsigned int msgLen ) {
    m_queue = xQueueCreate( msgLen, sizeof( BusMsg* ) );
    assert( ( "Can't create queue", NULL != m_queue ) );
    Bus::get().connect( this );
}

IBusClient::~IBusClient() {
    leaveBus();
    vQueueDelete( m_queue );
}

void IBusClient::leaveBus() {
    Bus::get().disconnect( this );
    BusMsg* raw;
    while( pdTRUE == xQueueReceive( queue(), (void*)&raw, (portTickType)0 ) && raw ) {
        raw->unref();
    }
}

void IBusClient::send( const char* to, std::unique_ptr<IBusCommand> msg ) const {
    if( to && msg ) {
        msg->isGroup = true;
        send( Bus::get().nameToAddr( to ), std::move( msg ) );
    }
}

void IBusClient::send( const BusAddr to, std::unique_ptr<IBusCommand> msg ) const {
    if( BusAddrInvalid == to || !msg ) {
        return;
    }

    msg->setFromTo( addr(), to );
    sendToClients( to, std::move( msg ) );
}

void IBusClient::tryReceive( TickType_t waitTime ) {
    BusMsg* raw;
    while( pdTRUE == xQueueReceive( queue(), (void*)&raw, (portTickType)waitTime ) ) {
        receive( raw->from, raw->to, *raw );
        raw->unref();
    }
}

bool IBusClient::tryReceiveSingleMessage( TickType_t waitTime ) {
    BusMsg* raw;
    if( pdTRUE != xQueueReceive( queue(), (void*)&raw, (portTickType)waitTime ) ) {
        return false;
    }

    receive( raw->from, raw->to, *raw );
    raw->unref();
    return true;
}

void IBusClient::sendToClients( BusAddr to, std::unique_ptr<BusMsg> busMsg ) {
    std::lock_guard<decltype( Bus::get().mutex() )> lock( Bus::get().mutex() );

    auto raw = busMsg.release();
    if( raw->isGroup ) {
        const auto& dests = Bus::get().getClientsByGroupAddr( to );
        for( const auto client : dests ) {
            raw->m_refCtr += 1;
            if( pdTRUE != xQueueSend( client->queue(), (void*)&raw, (portTickType)0 ) ) {
                raw->unref();
                constexpr auto depth = 100;
                esp_backtrace_print( depth );
            }
        }
    }
    else {
        auto client = Bus::get().getClientByAddr( to );
        if( client ) {
            raw->m_refCtr += 1;
            if( pdTRUE != xQueueSend( client->queue(), (void*)&raw, (portTickType)0 ) ) {
                raw->unref();
                constexpr auto depth = 100;
                esp_backtrace_print( depth );
            }
        }
    }

    raw->unref();
}

}  // namespace bus
