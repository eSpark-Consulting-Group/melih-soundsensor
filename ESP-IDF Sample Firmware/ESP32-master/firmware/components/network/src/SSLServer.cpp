#include "SSLServer.h"

#include "TCPServer2.h"
#include "SSLSocket.h"

#define ENABLE_SSLSRV_LOGS
#ifdef ENABLE_SSLSRV_LOGS

#include "DebugUtils.h"

#define SSLSRV_LOG_TAG "[SSL_SRV"
#define SSLSRV_LOGI( _FMT, ... ) ESP_LOGI( SSLSRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define SSLSRV_LOGE( _FMT, ... ) ESP_LOGE( SSLSRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define SSLSRV_LOGW( _FMT, ... ) ESP_LOGW( SSLSRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )

#else  // not ENABLE_SSLSRV_LOGS

#define SSLSRV_LOGI( ... ) \
    do {                   \
    } while( false )
#define SSLSRV_LOGE( ... ) \
    do {                   \
    } while( false )
#define SSLSRV_LOGW( ... ) \
    do {                   \
    } while( false )

#endif  // ENABLE_SSLSRV_LOGS

namespace network {

SSLServer::SSLServer( BlobType pubCert, BlobType privKey )
: m_pubCert( std::move( pubCert ) )
, m_privKey( std::move( privKey ) ) {
    SSLSRV_LOGI( "Call SSLServer()" );
}

SSLServer::~SSLServer() {
    SSLSRV_LOGI( "Call destructor" );
}

bool SSLServer::listen( StringType locAddr, std::uint16_t locPort ) {
    if( locAddr.empty() ) {
        SSLSRV_LOGI( "Call listen(locAddr=nullptr, locPort=%d)", locPort );
    }
    else {
        SSLSRV_LOGI( "Call listen(locAddr=%s, locPort=%d)", locAddr.c_str(), locPort );
    }

    m_server = std::make_unique<TCPServer2>();
    return m_server && m_server->listen( locAddr, locPort );
}

ISocket::UPtr SSLServer::nextPendingConnection() {
    auto sock = m_server->nextPendingConnection();
    if( !sock ) {
        return nullptr;
    }

    SSLSRV_LOGI( "Accept new connection" );
    auto sslSock = std::make_unique<SSLSocket>( std::move( sock ) );
    if( !sslSock ) {
        return nullptr;
    }

    sslSock->setCertificates( m_pubCert.data(), m_privKey.data(), nullptr );

    if( !sslSock->connectToClient() ) {
        SSLSRV_LOGE( "Can't connect to client!" );
        return nullptr;
    }

    SSLSRV_LOGI( "Handshake successful, connection established" );
    return std::move( sslSock );
}

}  // namespace network
