#include "ConnectionIDDataVer1.h"
#include <cstring>
#include <cassert>

namespace core {

ConnectionIDDataVer1::ConnectionIDDataVer1( const char* buff, std::size_t flash_offset )
: m_buff( buff )
, m_flash_offset( flash_offset ) {
    assert( m_buff );
}

const char* ConnectionIDDataVer1::provisioningServerURL() const {
    return m_buff + PROVISIONING_SERVER_URL_OFFSET;
}

const char* ConnectionIDDataVer1::provisioningServerToken() const {
    return m_buff + PROVISIONING_SERVER_TOKEN_OFFSET;
}

const char* ConnectionIDDataVer1::cloudServerURL() const {
    return m_buff + CLOUD_SERVER_URL_OFFSET;
}

const char* ConnectionIDDataVer1::CACertificate() const {
    return m_buff + CA_CERTIFICATE_OFFSET;
}

const char* ConnectionIDDataVer1::TLSPrivateKey() const {
    return m_buff + TLS_PRIVATE_KEY_OFFSET;
}

const char* ConnectionIDDataVer1::TLSPublicCertificate() const {
    return m_buff + TLS_PUBLIC_CERTIFICATE_OFFSET;
}

int ConnectionIDDataVer1::configRevision() const {
    // There is no such field in ver 1 so return safe number
    return 1;
}

}  // namespace core
