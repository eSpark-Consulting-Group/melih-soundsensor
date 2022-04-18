
#include "TCPSocket.h"

#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <cstring>
#include <string.h>
#include <errno.h>
#include <unistd.h>

//#define ENABLE_TCP_SOCK_LOGS
#ifdef ENABLE_TCP_SOCK_LOGS

#include "DebugUtils.h"

#define TSOCK_LOG_TAG "[TCP_SOCK"
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

TCPSocket::TCPSocket( const int sockId )
: m_sock( sockId ) {
    TSOCK_LOGI( "Constructor()" );
    if( BadSocket != sockId ) {
        setState( States::ConnectedState );
        m_readTimeout = getRecvTimeval();
        m_writeTimeout = getSendTimeval();
    }
}

TCPSocket::~TCPSocket() {
    flush();
    close();
    TSOCK_LOGI( "Destructor" );
}

bool TCPSocket::bind( const char* locAddr, const uint16_t locPort ) {
    TSOCK_LOGI( "Try bind to addr=%s port=%d", ( locAddr ? locAddr : "ANY" ), locPort );
    if( !createSocket() ) {
        return false;
    }

    m_sockAddr = {};
    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_port = ::htons( locPort );

    if( !locAddr ) {
        TSOCK_LOGI( "Bind to addr=ANY, port=%d", locPort );
        m_sockAddr.sin_addr.s_addr = ::htonl( INADDR_ANY );
    }
    else if( 1 != inet_pton( AF_INET, locAddr, &m_sockAddr.sin_addr ) && !setDestination( locAddr ) ) {
        return false;
    }

    const int TRUE = 1;
    if( 0 != ::setsockopt( m_sock, SOL_SOCKET, SO_REUSEADDR, &TRUE, sizeof( TRUE ) ) ) {
        TSOCK_LOGI( "Can't set SO_REUSEADDR" );
        return false;
    }

    if( NotError == ::bind( m_sock, (sockaddr*)&m_sockAddr, sizeof( m_sockAddr ) ) ) {
        setState( States::BoundState );
        TSOCK_LOGI( "Bind successfully" );
        return true;
    }

    TSOCK_LOGE( "Bind error: errno=%d, %s", errno, ::strerror( errno ) );
    return false;
}

bool TCPSocket::connectToHost( const char* remoteAddress, const uint16_t remotePort ) {
    if( remoteAddress ) {
        TSOCK_LOGI( "Call connectToHost: addr=%s, port=%d", remoteAddress, remotePort );
    }
    else {
        TSOCK_LOGE( "Call connectToHost error: bad remoteAddress" );
        return false;
    }

    if( !setDestination( remoteAddress ) ) {
        return false;
    }
    m_sockAddr.sin_port = ::htons( remotePort );

    if( !createSocket() ) {
        return false;
    }

    auto retCode = ::connect( m_sock, (sockaddr*)&m_sockAddr, sizeof( m_sockAddr ) );
    if( NotError == retCode ) {
        setState( States::ConnectedState );
        TSOCK_LOGI( "Connected to host successfully!" );
        return true;
    }

    TSOCK_LOGE( "Connecting to host failed, retCode=%d, errno=%d, %s", retCode, errno, ::strerror( errno ) );
    return false;
}

int TCPSocket::send( const char* buffer, const std::size_t bufSize, const int flags ) {
    // TSOCK_LOGI( "Call send: dataSize=%d", size );

    int numBytes = ::send( m_sock, buffer, bufSize, flags );
    if( 0 > numBytes ) {
        if( EAGAIN == errno ) {
            // TSOCK_LOGI( "send: Can't send data, try again" );
            return 0;
        }
        else if( EWOULDBLOCK == errno ) {
            TSOCK_LOGW( "send: Can't send data, because blocked" );
            return 0;
        }

        TSOCK_LOGW( "send: errCode=%d, errDescr=%s", errno, ::strerror( errno ) );
        close();
    }

    return numBytes;
}

int TCPSocket::receive( char* buffer, const std::size_t bufSize ) {
    // TSOCK_LOGI( "Call receive: dataSize=%d", size );

    auto numBytes = ::recv( m_sock, buffer, bufSize, 0 );
    if( 0 > numBytes ) {
        if( EAGAIN == errno ) {
            // TSOCK_LOGI( "receive: Can't receive data, try again" );
            return 0;
        }
        else if( EWOULDBLOCK == errno ) {
            TSOCK_LOGW( "receive: Can't receive data, because blocked" );
            return 0;
        }

        TSOCK_LOGW( "receive: errCode=%d, errDescr=%s", errno, ::strerror( errno ) );
        close();
    }

    return numBytes;
}

bool TCPSocket::createSocket() {
    TSOCK_LOGI( "Call createSocket()" );

    if( BadSocket != m_sock && !close() ) {
        TSOCK_LOGW( "createSocket: Socket already opened! Close it and create new!" );
        return false;
    }

    m_sock = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( BadSocket == m_sock ) {
        TSOCK_LOGE( "createSocket: errCode=%d, errDescr=%s", errno, ::strerror( errno ) );
        return false;
    }

    m_readTimeout = getRecvTimeval();
    m_writeTimeout = getSendTimeval();

    TSOCK_LOGI( "createSocket: opened successfully" );
    return true;
}

bool TCPSocket::close() {
    TSOCK_LOGI( "Call close()" );

    if( BadSocket == m_sock ) {
        TSOCK_LOGW( "Socket already closed" );
        return false;
    }

    auto errCode = ::close( m_sock );
    m_sock = BadSocket;
    setState( States::UnconnectedState );
    if( 0 > errCode ) {
        TSOCK_LOGE( "Failed to close:  errCode=%d, errDescr=%s", errno, ::strerror( errno ) );
        return false;
    }

    TSOCK_LOGI( "Closed successfully" );
    return true;
}

bool TCPSocket::setDestination( const char* remoteAddr ) {
    TSOCK_LOGI( "Call setDestination(addr=%s)", remoteAddr );

    auto hostInfo = ::gethostbyname( remoteAddr );
    if( !hostInfo ) {
        TSOCK_LOGE( "gethostbyname failed, error: %s(%d)", strerror( errno ), errno );
        return false;
    }

    m_sockAddr.sin_family = hostInfo->h_addrtype;
    if( AF_INET != hostInfo->h_addrtype ) {
        TSOCK_LOGW( "Not IPv4 address type" );
    }

    TSOCK_LOGW( "Resolved '%s' => '%s'", remoteAddr, inet_ntoa( *(in_addr*)( hostInfo->h_addr ) ) );
    memcpy( &m_sockAddr.sin_addr.s_addr, hostInfo->h_addr, sizeof( m_sockAddr.sin_addr.s_addr ) );
    return true;
}

bool TCPSocket::setReadTimeout( const SystemTimeval& timeout ) {
    // TSOCK_LOGI( "Try set read timeout %ld(sec), %ld(usec)", timeout.getSec(), timeout.getUsec() );

    if( timeout == m_readTimeout ) {
        return true;
    }

    if( 0 == ::setsockopt( m_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof( timeout ) ) ) {
        TSOCK_LOGI( "Set read timeout %ld(sec), %ld(usec)", timeout.getSec(), timeout.getUsec() );
        m_readTimeout = timeout;
        return true;
    }

    TSOCK_LOGE( "Cannot set reading timeout errorCode=%d, errorStr=%s", errno, ::strerror( errno ) );
    return false;
}

bool TCPSocket::setWriteTimeout( const SystemTimeval& timeout ) {
    // TSOCK_LOGI( "Try set write timeout %ld(sec), %ld(usec)", timeout.getSec(), timeout.getUsec() );

    if( timeout == m_writeTimeout ) {
        return true;
    }

    if( 0 == ::setsockopt( m_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof( timeout ) ) ) {
        TSOCK_LOGI( "Set write timeout %ld(sec), %ld(usec)", timeout.getSec(), timeout.getUsec() );
        m_writeTimeout = timeout;
        return true;
    }

    TSOCK_LOGE( "Cannot set writing timeout errorCode=%d, errorStr=%s", errno, ::strerror( errno ) );
    return false;
}

bool TCPSocket::setBlocking( bool blocking ) {
    TSOCK_LOGI( "Set blocking to %d", (int)blocking );

    if( BadSocket == m_sock ) {
        return false;
    }

    const int flags = ::fcntl( m_sock, F_GETFL, 0 );
    return 0 == ::fcntl( m_sock, F_SETFL, blocking ? flags & ( ~O_NONBLOCK ) : flags | O_NONBLOCK );
}

::timeval TCPSocket::getSendTimeval() const {
    ::timeval timeout;
    ::socklen_t size;
    if( 0 == ::getsockopt( m_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, &size ) ) {
        TSOCK_LOGI( "getSendTimeval: %ld(sec), %ld(msec)", timeout.tv_sec, ( timeout.tv_usec / 1000 ) );
        return timeout;
    }

    TSOCK_LOGE( "getSendTimeval failed: errCode=%d, errDescr=%s", errno, ::strerror( errno ) );
    timeout = {};
    return timeout;
}

::timeval TCPSocket::getRecvTimeval() const {
    ::timeval timeout;
    ::socklen_t size;
    if( 0 == ::getsockopt( m_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, &size ) ) {
        TSOCK_LOGI( "getRecvTimeval: %ld(sec), %ld(msec)", timeout.tv_sec, ( timeout.tv_usec / 1000 ) );
        return timeout;
    }

    TSOCK_LOGE( "getRecvTimeval failed: errCode=%d, errDescr=%s", errno, ::strerror( errno ) );
    timeout = {};
    return timeout;
}

}  // namespace network
