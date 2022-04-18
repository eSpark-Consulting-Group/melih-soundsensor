#include "sdkconfig.h"

#include "SSLSocket.h"
#include "TCPSocket.h"

#ifdef CONFIG_MBEDTLS_HAVE_TIME
#include "time_service/Utils.h"
#endif  // CONFIG_MBEDTLS_HAVE_TIME

//#define ENABLE_SSL_SOCK_LOGS
#ifdef ENABLE_SSL_SOCK_LOGS

#include "DebugUtils.h"

#define SSOCK_LOG_TAG "[SSL_SOCK"
#define SSOCK_LOGI( _FMT, ... ) ESP_LOGI( SSOCK_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define SSOCK_LOGE( _FMT, ... ) ESP_LOGE( SSOCK_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define SSOCK_LOGW( _FMT, ... ) ESP_LOGW( SSOCK_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )

#else  // not ENABLE_SSL_SOCK_LOGS

#define SSOCK_LOGI( ... ) \
    do {                  \
    } while( false )
#define SSOCK_LOGE( ... ) \
    do {                  \
    } while( false )
#define SSOCK_LOGW( ... ) \
    do {                  \
    } while( false )

#endif  // ENABLE_TCP_SOCK_LOGS

namespace network {

using namespace std::chrono_literals;

namespace {
int sendClbk( ISocket* socket, const char* buffer, const std::size_t size ) {
    return socket->send( (const char*)buffer, size );
};

int receiveClbk( ISocket* socket, char* buffer, const std::size_t size ) {
    return socket->receive( (char*)buffer, size );
};
}  // namespace

SSLSocket::SSLSocket( ISocket::UPtr&& socket )
: ProxySocket( std::move( socket ) ) {
    SSOCK_LOGI( "Constructor(ISocket&&)" );
}

SSLSocket::SSLSocket()
: SSLSocket( std::make_unique<TCPSocket>() ) {
    SSOCK_LOGI( "Constructor()" );
}

SSLSocket::~SSLSocket() {
    SSOCK_LOGI( "Destructor" );
}

bool SSLSocket::connectToHostViaProxySocket( const char* remoteAddress, uint16_t remotePort ) {
    if( remoteAddress ) {
        SSOCK_LOGI( "Call connectToHostViaProxySocket: addr=%s, port=%d", remoteAddress, remotePort );
    }
    else {
        SSOCK_LOGE( "Call connectToHostViaProxySocket error: bad remoteAddress" );
        return false;
    }

#ifdef CONFIG_MBEDTLS_HAVE_TIME
    if( !time_service::waitingTimeDidEstablishedFor( 0ms ) ) {
        return false;
    }
#endif  // CONFIG_MBEDTLS_HAVE_TIME

    auto tlsCtx = std::make_unique<TLSContext>();
    if( !tlsCtx || !tlsCtx->setCertificates( m_caCert, m_pKey, m_publicCert ) || !tlsCtx->initClient() ) {
        return false;
    }

    if( mbedtls_ssl_set_hostname( &tlsCtx->ssl, remoteAddress ) != 0 ) {
        return false;
    }
    mbedtls_ssl_set_bio( &tlsCtx->ssl,
                         &socket(),
                         reinterpret_cast<mbedtls_ssl_send_t*>( sendClbk ),
                         reinterpret_cast<mbedtls_ssl_recv_t*>( receiveClbk ),
                         NULL );
    if( !tlsCtx->runHandshake() ) {
        return false;
    }
    m_tlsCtx = std::move( tlsCtx );

    return true;
}

bool SSLSocket::connectToClient() {
    SSOCK_LOGI( "Call connectToClient()" );

    auto tlsCtx = std::make_unique<TLSContext>();
    if( !tlsCtx || !tlsCtx->setCACert( m_caCert ) || !tlsCtx->setPrivKey( m_pKey ) || !tlsCtx->initServer() ) {
        return false;
    }

    mbedtls_ssl_set_bio( &tlsCtx->ssl,
                         &socket(),
                         reinterpret_cast<mbedtls_ssl_send_t*>( sendClbk ),
                         reinterpret_cast<mbedtls_ssl_recv_t*>( receiveClbk ),
                         NULL );

    m_tlsCtx = std::move( tlsCtx );
    setState( States::BoundState );
    return true;
}

bool SSLSocket::close() {
    SSOCK_LOGI( "close" );

    bool isSuccess = true;

    if( socket().isConnected() && isConnected() ) {
        if( 0 > mbedtls_ssl_close_notify( &m_tlsCtx->ssl ) ) {
            isSuccess = false;
        }
    }

    m_tlsCtx.reset();

    if( !ProxySocket::close() ) {
        isSuccess = false;
    }

    return isSuccess;
}

int SSLSocket::receive( char* buffer, const std::size_t bufferSize ) {
    // SSOCK_LOGI( "Call receive: dataSize=%d", bufferSize );
    auto retCode = mbedtls_ssl_read( &m_tlsCtx->ssl, (unsigned char*)buffer, bufferSize );
    if( 0 <= retCode ) {
        return retCode;
    }

    if( retCode == MBEDTLS_ERR_SSL_WANT_READ || retCode == MBEDTLS_ERR_SSL_WANT_WRITE ) {
        return 0;
    }

    close();

    return retCode;
}

int SSLSocket::send( const char* buffer, const std::size_t bufferSize, const int flags ) {
    // SSOCK_LOGI( "Call send: dataSize=%d", bufferSize );
    auto retCode = mbedtls_ssl_write( &m_tlsCtx->ssl, (const unsigned char*)buffer, bufferSize );
    if( 0 <= retCode ) {
        return retCode;
    }

    if( retCode == MBEDTLS_ERR_SSL_WANT_READ || retCode == MBEDTLS_ERR_SSL_WANT_WRITE ) {
        return 0;
    }

    close();

    return retCode;
}

}  // namespace network
