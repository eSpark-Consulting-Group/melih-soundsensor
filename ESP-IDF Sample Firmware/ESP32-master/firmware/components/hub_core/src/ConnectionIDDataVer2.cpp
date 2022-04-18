#include <ConnectionIDDataVer2.h>
#include <cstring>
#include <FlashUtils.h>

namespace core {

ConnectionIDDataVer2::ConnectionIDDataVer2( const char* buff, std::size_t flash_offset )
: ConnectionIDDataVer1( buff, flash_offset ) {}

int ConnectionIDDataVer2::configRevision() const {
    int res;
    char buff[std::max( sizeof( res ), CONFIG_REVISION_SIZE )];
    std::memset( buff, 0, sizeof( buff ) );
    std::memcpy( buff, m_buff + CONFIG_REVISION_OFFSET, CONFIG_REVISION_SIZE );
    res = *( (int*)( buff ) );
    return res;
}

bool ConnectionIDDataVer2::setProvisioningServerURL( const std::string& provision_url ) {
    if( provision_url.size() >= PROVISIONING_SERVER_URL_MAX_SIZE ) {
        return false;
    }

    return hard::writeToFlash(
        m_flash_offset + PROVISIONING_SERVER_URL_OFFSET, provision_url.c_str(), provision_url.size() + 1 );
}

bool ConnectionIDDataVer2::setProvisioningServerToken( const std::string& provision_token ) {
    if( provision_token.size() > PROVISIONING_SERVER_TOKEN_SIZE ) {
        return false;
    }
    return hard::writeToFlash(
        m_flash_offset + PROVISIONING_SERVER_TOKEN_OFFSET, provision_token.c_str(), provision_token.size() );
}

bool ConnectionIDDataVer2::setCloudServerURL( const std::string& cloud_url ) {
    if( cloud_url.size() >= CLOUD_SERVER_URL_MAX_SIZE ) {
        return false;
    }
    return hard::writeToFlash( m_flash_offset + CLOUD_SERVER_URL_OFFSET, cloud_url.c_str(), cloud_url.size() + 1 );
}

bool ConnectionIDDataVer2::setCACertificate( const std::string& ca_cert ) {
    if( ca_cert.size() >= CA_CERTIFICATE_MAX_SIZE ) {
        return false;
    }
    return hard::writeToFlash( m_flash_offset + CA_CERTIFICATE_OFFSET, ca_cert.c_str(), ca_cert.size() + 1 );
}

bool ConnectionIDDataVer2::setTLSPrivateKey( const std::string& private_key ) {
    if( private_key.size() >= TLS_PRIVATE_KEY_MAX_SIZE ) {
        return false;
    }
    return hard::writeToFlash( m_flash_offset + TLS_PRIVATE_KEY_OFFSET, private_key.c_str(), private_key.size() + 1 );
}

bool ConnectionIDDataVer2::setTLSPublicCertificate( const std::string& public_cert ) {
    if( public_cert.size() >= TLS_PUBLIC_CERTIFICATE_MAX_SIZE ) {
        return false;
    }
    return hard::writeToFlash(
        m_flash_offset + TLS_PUBLIC_CERTIFICATE_OFFSET, public_cert.c_str(), public_cert.size() + 1 );
}

bool ConnectionIDDataVer2::setConfigRevision( int rev ) {
    char buff[CONFIG_REVISION_SIZE];
    std::memset( buff, 0, sizeof( buff ) );
    std::memcpy( buff, &rev, std::min( sizeof( rev ), sizeof( buff ) ) );
    return hard::writeToFlash( m_flash_offset + CONFIG_REVISION_OFFSET, buff, CONFIG_REVISION_SIZE );
}

}  // namespace core
