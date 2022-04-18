#include "WebSocketServer.h"
#include "JsonSocket.h"
#include <Ezlo.h>
#include "ProvisionPeersInfoManager.h"
#include <StreamDigestCalculator.h>

#include <memory>
#include <algorithm>
#include <regex.h>
#include <string.h>

#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>
#include "mbedtls/ssl.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <DebugUtils.h>

namespace network {

WSServer::WSServer( BlobType pubCert, BlobType privKey )
: m_serv( std::move( pubCert ), std::move( privKey ) ) {
    FUNC_LOGI( "WSServer constructor" );
}

WSServer::~WSServer() {
    FUNC_LOGI( "WSServer destructor" );
}

bool WSServer::listen( const StringType locAddr, const std::uint16_t locPort ) {
    return m_serv.listen( locAddr, locPort );
}

ISocket::UPtr WSServer::nextPendingConnection() {
    auto sock = m_serv.nextPendingConnection();
    if( !sock ) {
        return nullptr;
    }
    FUNC_LOGI( "Accept new connection" );

    if( !sock->setReadTimeout( std::chrono::seconds( 3 ) ) || !sock->setWriteTimeout( std::chrono::seconds( 3 ) ) ) {
        return nullptr;
    }

    constexpr auto bufferSize = 4096;  // We except size of request < bufferSize - 1  bytes
    auto buffer = std::make_unique<char[]>( bufferSize );

    int ret = sock->receive( buffer.get(), bufferSize - 1 );
    if( 0 >= ret ) {
        return nullptr;
    }
    buffer[ret] = '\0';

    if( strstr( buffer.get(), "\r\n\r\n" ) == nullptr ) {
        return nullptr;
    }
    FUNC_LOGI( "Receive:\n%s", buffer.get() );

    initialRequestInfo requestFields;
    if( !parseClientRequest( buffer.get(), bufferSize, requestFields ) ) {
        return nullptr;
    }
    FUNC_LOGI(
        "Path:\n%s\nKey:\n%s\nVersion:\n%d", requestFields.path, requestFields.key, requestFields.secWebsocketWersion );

    if( requestFields.secWebsocketWersion != 13 ) {
        // TODO:generate 400 Bad Request answer
        return nullptr;
    }

    ret = generateHandshakeResponse( buffer.get(), bufferSize, requestFields.key );
    if( ret < 0 ) {
        return nullptr;
    }

    ret = sock->send( buffer.get(), ret );
    if( ret < 0 ) {
        return nullptr;
    }

    constexpr bool isSendMask{ false };
    return std::make_unique<JsonSocket>( std::move( sock ), isSendMask );
}

bool WSServer::parseClientRequest( char* buffer, uint16_t size, initialRequestInfo& requestInfo ) {
    if( strstr( buffer, "\r\n\r\n" ) == nullptr ) {
        assert( false );
        return false;
    }
    // split request
    char *ptrStart, *ptrEnd;
    uint16_t countOfLines = 0;
    char* lines[16];
    ptrStart = buffer;

    while( true ) {
        ptrEnd = strstr( ptrStart, "\r\n" );
        if( ptrEnd == nullptr ) {
            assert( false );
            return false;
        }
        lines[countOfLines] = ptrStart;
        countOfLines++;
        ptrEnd[0] = '\0';
        ptrEnd[1] = '\0';
        if( ptrEnd[2] == '\r' && ptrEnd[3] == '\n' ) {
            break;
        }
        ptrStart = ptrEnd + 2;
    }

    // GET header
    if( strncmp( lines[0], "GET ", sizeof( "GET " ) - 1 ) != 0 ) {
        assert( false );
        return false;
    }
    ptrStart = lines[0] + 4;
    ptrEnd = strstr( lines[0], "HTTP/1.1" );

    if( ptrEnd == nullptr ) {
        assert( false );
        return false;
    }

    std::copy( ptrStart, ptrEnd, requestInfo.path );
    requestInfo.path[ptrEnd - ptrStart] = '\0';

    // find "Upgrade: websocket" "Connection: Upgrade" headers
    bool founded = false;

    for( auto i = 1; i < countOfLines; ++i ) {
        if( strcmp( lines[i], "Upgrade: websocket" ) == 0 ) {
            founded = true;
            break;
        }
    }

    if( !founded ) {
        assert( false );
        return false;
    }

    founded = false;

    for( auto i = 1; i < countOfLines; ++i ) {
        if( strncmp( lines[i], "Connection: ", sizeof( "Connection: " ) - 1 ) == 0 ) {
            if( strstr( lines[i], "Upgrade" ) != nullptr ) {
                founded = true;
            }
        }
    }

    if( !founded ) {
        return false;
    }

    founded = false;

    for( auto i = 1; i < countOfLines; ++i ) {
        if( strncmp( lines[i], "Sec-WebSocket-Key: ", sizeof( "Sec-WebSocket-Key: " ) - 1 ) == 0 ) {
            constexpr auto keySize = 24;
            std::copy( lines[i] + sizeof( "Sec-WebSocket-Key: " ) - 1,
                       lines[i] + sizeof( "Sec-WebSocket-Key: " ) - 1 + keySize,
                       requestInfo.key );
            requestInfo.key[keySize] = '\0';
            founded = true;
        }
    }

    if( !founded ) {
        return false;
    }

    founded = false;

    for( auto i = 1; i < countOfLines; ++i ) {
        if( strncmp( lines[i], "Sec-WebSocket-Version: ", sizeof( "Sec-WebSocket-Version: " ) - 1 ) == 0 ) {
            requestInfo.secWebsocketWersion = atoi( lines[i] + sizeof( "Sec-WebSocket-Version: " ) - 1 );
        }
    }

    return true;
}

static constexpr char wsGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

int WSServer::generateHandshakeResponse( char* buffer, uint16_t size, const char* key ) {
    char hashBuf[20];
    char secAcceptBuf[32];
    core::StreamDigestCalculator calc( core::StreamDigestCalculator::DigestType::SHA1 );
    calc.updateDigest( key, 24 );
    calc.updateDigest( wsGuid, sizeof( wsGuid ) - 1 );
    calc.getDigest( hashBuf, 20 );
    size_t base64Written;
    auto ret = mbedtls_base64_encode( reinterpret_cast<unsigned char*>( secAcceptBuf ),
                                      32,
                                      &base64Written,
                                      reinterpret_cast<unsigned char*>( hashBuf ),
                                      20 );

    if( ret != 0 ) {
        return -1;
    }

    ret = snprintf( buffer,
                    size,
                    "HTTP/1.1 101 Switching Protocols\r\n"
                    "Upgrade: websocket\r\n"
                    "Connection: Upgrade\r\n"
                    "Sec-WebSocket-Accept: %s\r\n\r\n",
                    secAcceptBuf );

    if( ret == size - 1 ) {
        return -1;
    }

    return ret;
}

}  // namespace network
