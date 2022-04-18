#pragma once

#include "ISocket.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#include <string>

namespace network {

class TCPSocket : public ISocket {
public:
    using UPtr = std::unique_ptr<TCPSocket>;
    using SPtr = std::shared_ptr<TCPSocket>;

    enum { NotError = 0, BadSocket = -1 };

public:
    TCPSocket( const int sockId = BadSocket );

    ~TCPSocket() override;

public:
    bool bind( const char* localAddress = nullptr, const uint16_t localPort = 0 );

    bool connectToHost( const char* remoteAddress, const uint16_t remotePort ) override;

    int send( const char* buffer, const std::size_t size, const int flags = 0 ) override;

    int receive( char* buffer, const std::size_t size ) override;

    bool setReadTimeout( const SystemTimeval& timeout ) override;

    SystemTimeval getReadTimeval() const override {
        return m_readTimeout;
    }

    bool setWriteTimeout( const SystemTimeval& timeout ) override;

    SystemTimeval getWriteTimeval() const override {
        return m_writeTimeout;
    }

    bool close() override;

    bool setBlocking( bool blocking ) override;

    inline auto sockId() const {
        return m_sock;
    }

protected:
    bool createSocket();

    bool setDestination( const char* remoteAddress );

    ::timeval getSendTimeval() const;

    ::timeval getRecvTimeval() const;

protected:
    int m_sock;
    sockaddr_in m_sockAddr = {};

    SystemTimeval m_readTimeout, m_writeTimeout;
};

}  //  namespace network
