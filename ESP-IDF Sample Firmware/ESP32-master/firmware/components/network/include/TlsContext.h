#pragma once

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/x509.h"

#include <memory>

namespace network {

class TLSContext {
public:
    using UPtr = std::unique_ptr<TLSContext>;

    TLSContext();

    ~TLSContext();

    bool setCACert( const char* caCert );

    bool setPubCert( const char* pubCert );

    bool setPrivKey( const char* privKey );

    bool setCertificates( const char* caCert, const char* pKey, const char* publicCert ) {
        return setCACert( caCert ) && ( !( publicCert && pKey ) || ( setPubCert( publicCert ) && setPrivKey( pKey ) ) );
    }

    bool initClient();

    bool initServer();

    bool runHandshake();

public:
    mbedtls_ssl_context ssl;

private:
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_pk_context p_key;
    mbedtls_x509_crt own_cert;
};

}  // namespace network
