#pragma once

#include "SSLServer.h"
#include "ISocket.h"

#include <chrono>
#include <vector>

namespace network {

class WSServer : public IServer {
public:
    using UPtr = std::unique_ptr<WSServer>;
    using BlobType = std::vector<char>;

    WSServer( BlobType pubCert, BlobType privKey );

    ~WSServer();

public:  // IServer
    bool listen( const StringType locAddr = StringType(), const std::uint16_t locPort = 0 ) override;

    using IServer::listen;

    ISocket::UPtr nextPendingConnection() override;

    inline bool isListening() const override {
        return m_serv.isListening();
    }

    inline bool close() override {
        return m_serv.close();
    }

private:
    struct initialRequestInfo {
        char path[64];  // Resource path from GET request
        uint16_t secWebsocketWersion;
        char key[25];  // Sec-WebSocket-Key
    };

    bool parseClientRequest( char* buffer, uint16_t size, initialRequestInfo& requestInfo );

    int generateHandshakeResponse( char* buffer, uint16_t size, const char* key );

private:
    SSLServer m_serv;
};

}  // namespace network
