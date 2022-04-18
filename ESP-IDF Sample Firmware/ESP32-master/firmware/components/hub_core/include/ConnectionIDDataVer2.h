#pragma once
#include <ConnectionIDDataVer1.h>
#include <string>

namespace core {

class ConnectionIDDataVer2 : public ConnectionIDDataVer1 {
public:
    ConnectionIDDataVer2() = delete;
    ConnectionIDDataVer2( const char* buff, std::size_t flash_offset );
    ConnectionIDDataVer2( const ConnectionIDDataVer2& other ) = delete;
    ConnectionIDDataVer2( ConnectionIDDataVer2&& other ) = delete;
    ConnectionIDDataVer2& operator=( const ConnectionIDDataVer2& other ) = delete;
    ConnectionIDDataVer2& operator=( ConnectionIDDataVer2&& other ) = delete;
    virtual ~ConnectionIDDataVer2() override = default;
    int configRevision() const override;

    bool setProvisioningServerURL( const std::string& provision_url );
    bool setProvisioningServerToken( const std::string& provision_token );
    bool setCloudServerURL( const std::string& cloud_url );
    bool setCACertificate( const std::string& ca_cert );
    bool setTLSPrivateKey( const std::string& private_key );
    bool setTLSPublicCertificate( const std::string& public_cert );
    bool setConfigRevision( int rev );

public:
    static constexpr std::size_t CONFIG_REVISION_OFFSET = 772;
    static constexpr std::size_t CONFIG_REVISION_SIZE = 4;
};

}  // namespace core
