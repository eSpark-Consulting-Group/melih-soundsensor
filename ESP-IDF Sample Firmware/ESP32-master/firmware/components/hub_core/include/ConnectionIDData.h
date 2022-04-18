#pragma once

namespace core {

class ConnectionIDData {
public:
    virtual ~ConnectionIDData() = default;

    // Ver 1
    virtual const char* provisioningServerURL() const = 0;
    virtual const char* provisioningServerToken() const = 0;
    virtual const char* cloudServerURL() const = 0;
    virtual const char* CACertificate() const = 0;
    virtual const char* TLSPrivateKey() const = 0;
    virtual const char* TLSPublicCertificate() const = 0;
    // Ver 2
    virtual int configRevision() const = 0;
};

}  // namespace core
