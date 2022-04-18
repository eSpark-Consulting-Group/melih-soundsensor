#pragma once

#include "ISocket.h"
#include "TlsContext.h"

namespace network {

class SSLSocket : public ProxySocket {
public:
    using UPtr = std::unique_ptr<SSLSocket>;

    SSLSocket( ISocket::UPtr&& socket );

    SSLSocket();

    ~SSLSocket() override;

public:
    bool connectToClient();

    int send( const char* buffer, const std::size_t size, const int flags = 0 ) override;

    int receive( char* buffer, const std::size_t size ) override;

    bool close() override;

public:
    inline void setCertificates( const char* caCert, const char* pKey, const char* publicCert ) {
        m_caCert = caCert;
        m_pKey = pKey;
        m_publicCert = publicCert;
    }

private:
    bool connectToHostViaProxySocket( const char* remoteAddress, const uint16_t remotePort ) override;

private:
    const char* m_caCert;
    const char* m_pKey;
    const char* m_publicCert;
    TLSContext::UPtr m_tlsCtx;
};

}  // namespace network
