#pragma once

#include "IServer.h"
#include "ISocket.h"

#include <string>
#include <vector>

namespace network {

class SSLServer : public IServer {
public:
    using BlobType = std::vector<char>;

    SSLServer( BlobType pubCert, BlobType privKey );

    ~SSLServer();

    bool listen( const StringType locAddr = StringType(), const std::uint16_t locPort = 0 ) override;

    using IServer::listen;

    ISocket::UPtr nextPendingConnection() override;

    inline bool isListening() const override {
        return m_server && m_server->isListening();
    }

    inline bool close() override {
        return m_server && m_server->close();
    }

private:
    IServer::UPtr m_server;
    BlobType m_pubCert;
    BlobType m_privKey;
};

}  // namespace network
