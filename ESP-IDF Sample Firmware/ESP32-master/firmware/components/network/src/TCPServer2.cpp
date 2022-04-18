#include "TCPServer2.h"
#include "Thread.h"

#include "Ezlo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <thread>
#include <array>
#include <cstring>

#define ENABLE_TCP_SRV_LOGS
#ifdef ENABLE_TCP_SRV_LOGS

#include "DebugUtils.h"

#define TCP_SRV_LOG_TAG "[TCP_SRV"
#define TCP_SRV_LOGI( _FMT, ... ) ESP_LOGI( TCP_SRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define TCP_SRV_LOGE( _FMT, ... ) ESP_LOGE( TCP_SRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define TCP_SRV_LOGW( _FMT, ... ) ESP_LOGW( TCP_SRV_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )

#else

#define TCP_SRV_LOGI( ... ) \
    do {                    \
    } while( false )
#define TCP_SRV_LOGE( ... ) \
    do {                    \
    } while( false )
#define TCP_SRV_LOGW( ... ) \
    do {                    \
    } while( false )

#endif  // ENABLE_TCP_SRV_LOGS

namespace network {

bool TCPServer2::listen( const StringType address, const std::uint16_t port ) {
    bool isSuccess = address.empty() ? m_sock.bind( nullptr, port ) : m_sock.bind( address.c_str(), port );
    if( !isSuccess ) {
        TCP_SRV_LOGE( "Can't bind socket!" );
        return false;
    }

    if( !m_sock.setBlocking( false ) ) {
        TCP_SRV_LOGE( "Can't set socket to nonblock mode!" );
        return false;
    }

    if( 0 != ::listen( m_sock.sockId(), m_maxConn ) ) {
        TCP_SRV_LOGE( "Listen socket failed, errno=%d, descr=%s", errno, ::strerror( errno ) );
        return false;
    }

    m_isListening = true;
    return true;
}

ISocket::UPtr TCPServer2::nextPendingConnection() {
    auto retCode = accept( m_sock.sockId(), nullptr, nullptr );

    if( 0 > retCode ) {
        if( retCode == -1 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
            return nullptr;
        }
        else {
            TCP_SRV_LOGE( "Accept failed, errno=%d, %s", errno, ::strerror( errno ) );
            close();
            return nullptr;
        }
    }

    /*struct linger ling = { 1, 5 };
    if( 0 != ::setsockopt( retCode, SOL_SOCKET, SO_LINGER, &ling, sizeof( ling ) ) ) {
        TCP_SRV_LOGE( "setsockopt failed, errno=%d, %s", errno, ::strerror( errno ) );
        return nullptr;
    }*/

    TCP_SRV_LOGI( "Accepted connection sockId=%d", retCode );
    return std::make_unique<TCPSocket>( retCode );
}

void echoWorkerTask( ISocket::UPtr sock ) {
    FUNC_LOGW( "+++++++> start" );
    const char infoMsg[] = "\u001b[33mType \"quit\" string to close connection\u001b[0m\n";
    sock->sendAll( infoMsg, sizeof( infoMsg ) );
    auto buffer = std::make_unique<std::array<char, 256>>();
    while( true ) {
        auto recvCount = sock->receive( buffer->data(), buffer->size() );
        if( 0 > recvCount ) {
            FUNC_LOGE( "Socket error" );
            break;
        }
        else if( 0 < recvCount ) {
            *( buffer->data() + recvCount ) = '\0';
            FUNC_LOGI( "Received data: %s", buffer->data() );
            const char txt[] = "\u001b[32mecho:";
            const char unsetColorSeq[] = "\u001b[0m";
            sock->sendAll( txt, sizeof( txt ) ) && sock->sendAll( buffer->data(), recvCount ) &&
                sock->sendAll( unsetColorSeq, sizeof( unsetColorSeq ) );
            const char quitCmd[] = "quit";
            if( 0 == std::strncmp( buffer->data(), quitCmd, sizeof( quitCmd ) - 1 ) ) {
                break;
            }
        }
    }
    FUNC_LOGW( "-------< stop" );
}

void tcpEchoServerTask( TcpEchoServerTaskContext* ctx ) {
    while( true ) {
        if( !ctx->srv.isListening() ) {
            while( true ) {
                if( ctx->srv.listen( std::string(), ctx->port ) ) {
                    break;
                }

                ctx->srv.close();
                vTaskDelay( 1000 / portTICK_PERIOD_MS );
            }
        }

        {
            auto sock = ctx->srv.nextPendingConnection();
            bool isSuccess = sock && sock->setWriteTimeout( std::chrono::seconds( 1 ) ) &&
                             sock->setReadTimeout( std::chrono::seconds( 1 ) );
            if( isSuccess ) {
                std::thread task( echoWorkerTask, std::move( sock ) );
                task.detach();
            }
        }

        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }

    delete ctx;
    vTaskDelete( NULL );
}

}  // namespace network