#include <StreamDigestCalculator.h>
#include <DebugUtils.h>

namespace core {

void StreamDigestCalculator::sha1_ctx_deleter( mbedtls_sha1_context* ctx ) {
    if( ctx != nullptr ) {
        mbedtls_sha1_free( ctx );
    }
    delete ctx;
}

void StreamDigestCalculator::sha256_ctx_deleter( mbedtls_sha256_context* ctx ) {
    if( ctx != nullptr )
        mbedtls_sha256_free( ctx );
    delete ctx;
}

StreamDigestCalculator::StreamDigestCalculator( DigestType type )
: m_digestType( type )
, m_sha1Ctx( nullptr, sha1_ctx_deleter )
, m_sha256Ctx( nullptr, sha256_ctx_deleter ) {
    init();
}

bool StreamDigestCalculator::setType( DigestType type ) {
    if( type == DigestType::Last ) {
        FUNC_LOGE( "Digest type not supported" );
        return false;
    }
    m_digestType = type;
    init();
    return true;
}

void StreamDigestCalculator::init() {
    freePreviousCtx();
    switch( m_digestType ) {
    case DigestType::SHA256:
        m_sha256Ctx.reset( new mbedtls_sha256_context );
        mbedtls_sha256_init( m_sha256Ctx.get() );
        mbedtls_sha256_starts( m_sha256Ctx.get(), 0 );
        m_isInitialized = true;
        break;
    case DigestType::SHA1:
        m_sha1Ctx.reset( new mbedtls_sha1_context );
        mbedtls_sha1_init( m_sha1Ctx.get() );
        mbedtls_sha1_starts( m_sha1Ctx.get() );
        m_isInitialized = true;
        break;
    default:
        FUNC_LOGE( "Digest type not supported" );
        m_isInitialized = false;
    }
}

void StreamDigestCalculator::reset() {
    init();
}

bool StreamDigestCalculator::updateDigest( const char* buffer, size_t bufferSize ) {
    if( !m_isInitialized ) {
        FUNC_LOGE( "Calculator is not initialized" );
        return false;
    }
    switch( m_digestType ) {
    case DigestType::SHA256:
        mbedtls_sha256_update( m_sha256Ctx.get(), (unsigned char*)buffer, bufferSize );
        return true;
    case DigestType::SHA1:
        mbedtls_sha1_update( m_sha1Ctx.get(), (unsigned char*)buffer, bufferSize );
        return true;
    default:
        FUNC_LOGE( "Digest type not supported" );
        return false;
    }
}

bool StreamDigestCalculator::getDigest( char* buffer, size_t bufferSize ) {
    if( !m_isInitialized ) {
        FUNC_LOGE( "Calculator is not initialized" );
        return false;
    }
    switch( m_digestType ) {
    case DigestType::SHA256:
        if( bufferSize >= 32 ) {
            mbedtls_sha256_finish( m_sha256Ctx.get(), (unsigned char*)buffer );
            m_isInitialized = false;
            return true;
        }
        FUNC_LOGE( "Buffer is too small" );
        return false;
    case DigestType::SHA1:
        if( bufferSize >= 20 ) {
            mbedtls_sha1_finish( m_sha1Ctx.get(), (unsigned char*)buffer );
            m_isInitialized = false;
            return true;
        }
        FUNC_LOGE( "Buffer is too small" );
        return false;
    default:
        FUNC_LOGE( "Digest type not supported" );
        return false;
    }
}

StreamDigestCalculator::~StreamDigestCalculator() {}
void StreamDigestCalculator::freePreviousCtx() {
    switch( m_digestType ) {
    case DigestType::SHA1:
        m_sha1Ctx.reset();
        break;
    case DigestType::SHA256:
        m_sha256Ctx.reset();
        break;
    default:
        break;
    }
    m_isInitialized = false;
}
}  // namespace core
