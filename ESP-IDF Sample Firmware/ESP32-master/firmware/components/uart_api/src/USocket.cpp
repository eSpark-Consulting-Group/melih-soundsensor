
#include "USocket.h"

#include <inttypes.h>

#include <cstring>

#include <driver/uart.h>
#define ENABLE_TCP_SOCK_LOGS
#ifdef ENABLE_TCP_SOCK_LOGS

#include "DebugUtils.h"

#define TSOCK_LOG_TAG "[U_SOCK"
#define TSOCK_LOGI( _FMT, ... ) ESP_LOGI( TSOCK_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define TSOCK_LOGE( _FMT, ... ) ESP_LOGE( TSOCK_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define TSOCK_LOGW( _FMT, ... ) ESP_LOGW( TSOCK_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )

#else

#define TSOCK_LOGI( ... ) \
    do {                  \
    } while( false )
#define TSOCK_LOGE( ... ) \
    do {                  \
    } while( false )
#define TSOCK_LOGW( ... ) \
    do {                  \
    } while( false )

#endif  // ENABLE_TCP_SOCK_LOGS

namespace network {

USocket::USocket( const int sockId )
: m_sock( sockId ) {
    TSOCK_LOGI( "USocket" );

    setState( States::ConnectedState );

    uart_config_t uart_config = {};
    uart_config.baud_rate = 921600;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    int UartBufSize = 8 * 1024;

    uart_param_config( (uart_port_t)CONFIG_UART_PORT_NAPKO, &uart_config );
    uart_set_pin( (uart_port_t)CONFIG_UART_PORT_NAPKO, 2, 0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE );
    uart_driver_install(
        (uart_port_t)CONFIG_UART_PORT_NAPKO, 2 * UartBufSize + UART_FIFO_LEN, UartBufSize + UART_FIFO_LEN, 0, NULL, 0 );

    m_sock = 1;

    setState( States::UnconnectedState );
}

USocket::~USocket() {
    flush();
    uart_driver_delete( (uart_port_t)CONFIG_UART_PORT_NAPKO );
    TSOCK_LOGI( "Destructor" );
}

bool USocket::bind( const char* locAddr, const uint16_t locPort ) {
    if( !createSocket() ) {
        return false;
    }

    setState( States::BoundState );
    TSOCK_LOGI( "Bind OK" );
    return true;
}

bool USocket::connectToHost( const char* remoteAddress, const uint16_t remotePort ) {
    if( !createSocket() ) {
        return false;
    }
    setState( States::ConnectedState );
    return true;
}

int USocket::send( const char* buffer, const std::size_t bufSize, const int flags ) {
    int numBytes = uart_write_bytes( CONFIG_UART_PORT_NAPKO, (const char*)buffer, bufSize );
    if( 0 > numBytes ) {
        TSOCK_LOGW( "send: errCode=%d, errDescr=%s", errno, ::strerror( errno ) );
        close();
    }

    return numBytes;
}

int USocket::receive( char* buffer, const std::size_t bufSize ) {
    if( buffer == NULL || bufSize == 0 ) {
        TSOCK_LOGW( " buffer or bufSize == 0" );
        return 0;
    }
    auto numBytes = uart_read_bytes( CONFIG_UART_PORT_NAPKO, (uint8_t*)buffer, bufSize, 0 );
    if( 0 > numBytes ) {
        TSOCK_LOGW( "receive: errCode=%d, errDescr=%s", errno, ::strerror( errno ) );
        close();
    }
    else {
        //
    }

    return numBytes;
}

bool USocket::createSocket() {
    return true;
}

bool USocket::close() {
    TSOCK_LOGI( "Call close()" );
    return true;
}

bool USocket::setDestination( const char* remoteAddr ) {
    return true;
}

bool USocket::setReadTimeout( const SystemTimeval& timeout ) {
    return true;
}

bool USocket::setWriteTimeout( const SystemTimeval& timeout ) {
    return true;
}

bool USocket::setBlocking( bool blocking ) {
    return true;
}

}  // namespace network
