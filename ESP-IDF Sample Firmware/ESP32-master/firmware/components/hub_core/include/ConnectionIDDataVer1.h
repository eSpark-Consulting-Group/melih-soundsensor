#pragma once
#include <cstdint>
#include <ConnectionIDData.h>

namespace core {

class ConnectionIDDataVer1 : public ConnectionIDData {
public:
    ConnectionIDDataVer1() = delete;
    ConnectionIDDataVer1( const char* buff, std::size_t flash_offset );
    ConnectionIDDataVer1( const ConnectionIDDataVer1& other ) = delete;
    ConnectionIDDataVer1( ConnectionIDDataVer1&& other ) = delete;
    ConnectionIDDataVer1& operator=( const ConnectionIDDataVer1& other ) = delete;
    ConnectionIDDataVer1& operator=( ConnectionIDDataVer1&& other ) = delete;
    virtual ~ConnectionIDDataVer1() = default;

    const char* provisioningServerURL() const override;
    const char* provisioningServerToken() const override;
    const char* cloudServerURL() const override;
    const char* CACertificate() const override;
    const char* TLSPrivateKey() const override;
    const char* TLSPublicCertificate() const override;
    int configRevision() const override;

public:
    static constexpr std::size_t PROVISIONING_SERVER_URL_OFFSET = 4;
    static constexpr std::size_t PROVISIONING_SERVER_URL_MAX_SIZE = 256;
    static constexpr std::size_t PROVISIONING_SERVER_TOKEN_OFFSET = 260;
    static constexpr std::size_t PROVISIONING_SERVER_TOKEN_SIZE = 256;
    static constexpr std::size_t CLOUD_SERVER_URL_OFFSET = 516;
    static constexpr std::size_t CLOUD_SERVER_URL_MAX_SIZE = 256;

    // CERTIFICATES
    static constexpr std::size_t CA_CERTIFICATE_OFFSET = 0x1000;
    static constexpr std::size_t CA_CERTIFICATE_MAX_SIZE = 0x2000;
    static constexpr std::size_t TLS_PRIVATE_KEY_OFFSET = 0x3000;
    static constexpr std::size_t TLS_PRIVATE_KEY_MAX_SIZE = 0x1000;
    static constexpr std::size_t TLS_PUBLIC_CERTIFICATE_OFFSET = 0x4000;
    static constexpr std::size_t TLS_PUBLIC_CERTIFICATE_MAX_SIZE = 0x2000;

protected:
    const char* m_buff;
    std::size_t m_flash_offset;
};

}  // namespace core
