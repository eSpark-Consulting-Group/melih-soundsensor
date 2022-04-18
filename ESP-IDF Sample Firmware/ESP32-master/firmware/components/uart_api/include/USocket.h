#pragma once

#include "ISocket.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#include <string>

namespace network {

class USocket : public ISocket {
public:
    using UPtr = std::unique_ptr<USocket>;
    using SPtr = std::shared_ptr<USocket>;

    static constexpr auto NotError = 0;
    static constexpr auto BadSocket = -1;

public:
    USocket( const int sockId = BadSocket );

    ~USocket() override;

public:
    bool bind( const char* localAddress = nullptr, const uint16_t localPort = 0 );

    bool connectToHost( const char* remoteAddress, const uint16_t remotePort ) override;

    int send( const char* buffer, const std::size_t size, const int flags = 0 ) override;

    int receive( char* buffer, const std::size_t size ) override;

    bool setReadTimeout( const SystemTimeval& timeout ) override;

    SystemTimeval getReadTimeval() const override {
        return SystemTimeval();
    }

    bool setWriteTimeout( const SystemTimeval& timeout ) override;

    SystemTimeval getWriteTimeval() const override {
        return SystemTimeval();
    }

    bool close() override;

    bool setBlocking( bool blocking ) override;

    inline auto sockId() const {
        return m_sock;
    }

protected:
    bool createSocket();

    bool setDestination( const char* remoteAddress );

protected:
    int m_sock;
    sockaddr_in m_sockAddr = {};
};

}  //  namespace network
