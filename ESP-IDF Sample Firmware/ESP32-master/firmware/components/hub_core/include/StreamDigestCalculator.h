#pragma once
#include <mbedtls/sha256.h>
#include <mbedtls/sha1.h>
#include <memory>
namespace core {

// currently for SHA-256 only
class StreamDigestCalculator {
public:
    enum class DigestType { SHA256, SHA1, Last };

public:
    StreamDigestCalculator( DigestType = DigestType::SHA256 );
    ~StreamDigestCalculator();
    bool getDigest( char* buffer, size_t bufferSize );
    void reset();
    bool updateDigest( const char* buffer, size_t bufferSize );
    bool setType( DigestType type = DigestType::SHA256 );

private:
    void init();
    void freePreviousCtx();
    static void sha256_ctx_deleter( mbedtls_sha256_context* ctx );
    static void sha1_ctx_deleter( mbedtls_sha1_context* ctx );

private:
    DigestType m_digestType = DigestType::Last;
    std::unique_ptr<mbedtls_sha1_context, void ( * )( mbedtls_sha1_context* )> m_sha1Ctx;
    std::unique_ptr<mbedtls_sha256_context, void ( * )( mbedtls_sha256_context* )> m_sha256Ctx;
    mbedtls_sha256_context m_ctx;
    bool m_isInitialized = false;
};

}  // namespace core
