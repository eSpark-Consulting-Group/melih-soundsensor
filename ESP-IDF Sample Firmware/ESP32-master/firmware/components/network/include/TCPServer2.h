#pragma once

#include "IServer.h"
#include "TCPSocket.h"

#include <string>
#include <memory>

namespace network {

class TCPServer2 : public IServer {
public:
    using StringType = std::string;
    using UPtr = std::unique_ptr<TCPServer2>;

    TCPServer2( const int maxConn = 5 )
    : m_maxConn( maxConn ) {}

    ~TCPServer2() {
        close();
    }

    bool close() override {
        m_isListening = false;
        return m_sock.close();
    }

    inline bool isListening() const override {
        return m_isListening;
    }

    bool listen( const StringType address = StringType(), const std::uint16_t port = 0 ) override;

    using IServer::listen;

    ISocket::UPtr nextPendingConnection() override;

private:
    TCPSocket m_sock;
    bool m_isListening = false;
    int m_maxConn;
};

struct TcpEchoServerTaskContext {
    int port;
    TCPServer2 srv;

    TcpEchoServerTaskContext( int _port, int maxConn )
    : port( _port )
    , srv( maxConn ) {}
};

void tcpEchoServerTask( TcpEchoServerTaskContext* ctx );

}  // namespace network