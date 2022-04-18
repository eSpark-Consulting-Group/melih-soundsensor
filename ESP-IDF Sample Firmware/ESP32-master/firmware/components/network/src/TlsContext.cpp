#include "TlsContext.h"

#include <cstring>

/* Temporary disable
#include "mbedtls/error.h"
#ifdef CONFIG_MBEDTLS_DEBUG
#include <mbedtls/esp_debug.h>
#endif*/

//#define ENABLE_TLSCTX_SOCK_LOGS
#ifdef ENABLE_TLSCTX_SOCK_LOGS

#include "DebugUtils.h"

#define TLSCTX_LOG_TAG "[TLS_CTX"
#define TLSCTX_LOGI( _FMT, ... ) ESP_LOGI( TLSCTX_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define TLSCTX_LOGE( _FMT, ... ) ESP_LOGE( TLSCTX_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )
#define TLSCTX_LOGW( _FMT, ... ) ESP_LOGW( TLSCTX_LOG_TAG, "0x%04x] " _FMT, (unsigned int)this, ##__VA_ARGS__ )

#else  // not ENABLE_TLSCTX_SOCK_LOGS

#define TLSCTX_LOGI( ... ) \
    do {                   \
    } while( false )
#define TLSCTX_LOGE( ... ) \
    do {                   \
    } while( false )
#define TLSCTX_LOGW( ... ) \
    do {                   \
    } while( false )

#endif  // ENABLE_TLSCTX_SOCK_LOGS

namespace network {

TLSContext::TLSContext() {
    TLSCTX_LOGI( "Constructor: begin" );
    mbedtls_entropy_init( &entropy );
    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_x509_crt_init( &cacert );
    mbedtls_pk_init( &p_key );
    mbedtls_x509_crt_init( &own_cert );
    mbedtls_ssl_config_init( &conf );
    mbedtls_ssl_init( &ssl );
    TLSCTX_LOGI( "Constructor: end" );
}

TLSContext::~TLSContext() {
    TLSCTX_LOGI( "Destructor: begin" );
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_x509_crt_free( &cacert );
    mbedtls_pk_free( &p_key );
    mbedtls_x509_crt_free( &own_cert );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );
    TLSCTX_LOGI( "Destructor: end" );
}

bool TLSContext::setCACert( const char* caCert ) {
    TLSCTX_LOGI( "setCACert:" );
    if( caCert ) {
        TLSCTX_LOGI( "%s", caCert );

        if( 0 != mbedtls_x509_crt_parse(
                     &cacert, reinterpret_cast<const unsigned char*>( caCert ), std::strlen( caCert ) + 1 ) ) {
            TLSCTX_LOGE( "Can't parse tls CA cartificate" );
            return false;
        }
        // TLSCTX_LOGI( "Verification of SSL connection was enabled" );
        // mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_REQUIRED );
    }
    else {
        TLSCTX_LOGI( "nullptr" );

        TLSCTX_LOGW( "Verification of SSL connection was disabled" );
        mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_NONE );
    }

    return true;
}

bool TLSContext::setPubCert( const char* pubCert ) {
    TLSCTX_LOGI( "setPubCert" );
    if( pubCert ) {
        TLSCTX_LOGI( "%s", pubCert );
    }
    else {
        TLSCTX_LOGI( "nullptr" );
    }

    return pubCert &&
           0 == mbedtls_x509_crt_parse(
                    &own_cert, reinterpret_cast<const unsigned char*>( pubCert ), std::strlen( pubCert ) + 1 );
}

bool TLSContext::setPrivKey( const char* privKey ) {
    TLSCTX_LOGI( "setPrivKey" );
    if( privKey ) {
        TLSCTX_LOGI( "%s", privKey );
    }
    else {
        TLSCTX_LOGI( "nullptr" );
    }

    return privKey &&
           0 == mbedtls_pk_parse_key(
                    &p_key, reinterpret_cast<const unsigned char*>( privKey ), std::strlen( privKey ) + 1, NULL, 0 );
}

bool TLSContext::initClient() {
    TLSCTX_LOGI( "initClient" );

    auto retCode = mbedtls_ctr_drbg_seed( &ctr_drbg, ::mbedtls_entropy_func, &entropy, NULL, 0 );
    if( retCode != 0 ) {
        TLSCTX_LOGE( "mbedtls_ctr_drbg_seed failed %d(-0x%x)", retCode, -retCode );
        return false;
    }

    retCode = mbedtls_ssl_config_defaults(
        &conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT );
    if( retCode != 0 ) {
        TLSCTX_LOGE( "mbedtls_ssl_config_defaults failed %d(-0x%x)", retCode, -retCode );
        return false;
    }

    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );

    retCode = mbedtls_ssl_conf_own_cert( &conf, &own_cert, &p_key );
    if( retCode != 0 ) {
        TLSCTX_LOGE( "mbedtls_ssl_config_own_cert failed %d(-0x%x)", retCode, -retCode );
        return false;
    }
    /* Temporary disable
    #ifdef CONFIG_MBEDTLS_DEBUG
        mbedtls_esp_enable_debug_log( &m_tlsContext.conf, 4 );
    #endif*/

    retCode = mbedtls_ssl_setup( &ssl, &conf );
    if( retCode != 0 ) {
        TLSCTX_LOGE( "mbedtls_ssl_setup failed %d(-0x%x)", retCode, -retCode );
        return false;
    }

    return true;
}

bool TLSContext::initServer() {
    TLSCTX_LOGI( "initServer" );

    static constexpr auto SSL_PERSONALIZATION_DATA = "SSL_server";
    auto ret = mbedtls_ctr_drbg_seed( &ctr_drbg,
                                      mbedtls_entropy_func,
                                      &entropy,
                                      reinterpret_cast<const unsigned char*>( SSL_PERSONALIZATION_DATA ),
                                      strlen( SSL_PERSONALIZATION_DATA ) );
    if( 0 != ret ) {
        TLSCTX_LOGE( "Failed to set entropy source. Error code: %d", ret );
        return false;
    }

    if( ( ret = mbedtls_ssl_config_defaults(
              &conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 ) {
        TLSCTX_LOGE( "Failed to set SSL config defaults. Error code: %d", ret );
        return false;
    }

    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );

    if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &cacert, &p_key ) ) != 0 ) {
        TLSCTX_LOGE( "Failed to set SSL verification data. Error code: %d", ret );
        return false;
    }
    /* Temporary disable
    #ifdef CONFIG_MBEDTLS_DEBUG
        mbedtls_esp_enable_debug_log( &m_tlsCtx->conf, ESP_LOG_INFO );
    #endif*/
    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 ) {
        TLSCTX_LOGE( "Failed to set up SSL context. Error code: %d", ret );
        return false;
    }

    return true;
}

bool TLSContext::runHandshake() {
    TLSCTX_LOGI( "runHandshake" );

    auto retCode = mbedtls_ssl_handshake( &ssl );
    if( retCode != 0 ) {
        TLSCTX_LOGE( "mbedtls_ssl_handshake failed %d(-0x%x)", retCode, -retCode );
        return false;
    }

    // if( ( retCode = mbedtls_ssl_get_verify_result( &ssl ) ) != 0 ) {
    //    TLSCTX_LOGE( "Failed to verify peer certificate!" );
    //    return false;
    //}

    /* Temporary disable
    #ifdef SSL_FREE_CERTIFICATES_MEMORY_AFTER_CONNECTION
        mbedtls_x509_crt_free( &m_tlsContext.cacert );
    #endif*/
    return true;
}

}  // namespace network