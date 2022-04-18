#include "LocalNmaSrvClient.h"

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "ProvisionPeersInfoManager.h"
#include "NmaWebSocketHandler.h"

#include "mbedtls/md.h"
#include "mbedtls/sha256.h"

#include <algorithm>

#include "DebugUtils.h"

namespace local_nma_srv {

using namespace common::resources;
using namespace std::chrono_literals;

namespace {

struct LoginHandler : public nma::NmaJsonHandlerBase {
    LocalNmaClient& m_client;
    std::shared_ptr<::core::EspLogger> m_espLogger;
    ::gateways::IGatewayUserDataStorage& m_gatewayUserDataStorage;

    LoginHandler( LocalNmaClient& client,
                  std::shared_ptr<::core::EspLogger> espLogger,
                  ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage )
    : m_client{ client }
    , m_espLogger{ espLogger }
    , m_gatewayUserDataStorage{ gatewayUserDataStorage } {}

    ResponceType onHubOfflineLoginUi( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        if( !gen.writeOpeningObject() || !gen.writeClosingSymbol() ) {
            return std::make_tuple( errors::GenJsonFailed, StringType{} );
        }

        const auto params = json( tokens::Params );
        const auto user = params( "user" ).toString();
        const auto pass = params( "token" ).toString();
        if( isUser( user, pass ) ) {
            changeHandler( std::make_unique<nma::WebSocketHandler>( m_client, m_espLogger, m_gatewayUserDataStorage ) );
            return std::make_tuple( errors::NotError, StringType{} );
        }

        return std::make_tuple( errors::ApplicationError, errors::data::BadPassword );
    }

    ResponceType onHubOfflineLoginHub( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
        if( !gen.writeOpeningObject() || !gen.writeClosingSymbol() ) {
            return std::make_tuple( errors::GenJsonFailed, StringType{} );
        }

        const auto params = json( tokens::Params );
        const auto token = params( tokens::Token ).toString();
        if( token && verifyToken( token ) ) {
            return std::make_tuple( errors::NotError, StringType{} );
        }

        return std::make_tuple( errors::InvalidParams, StringType{} );
    }

    bool verifyToken( char* token ) {
        auto client = provision::peers::InfoManager::get();
        if( !client ) {
            return false;
        }
        const auto& tokens = client->getTokens();
        return std::find( tokens.begin(), tokens.end(), token ) != tokens.end();
    }

    bool isUser( char* user, char* pass ) {
        if( !user || !pass ) {
            FUNC_LOGI( "NOT USER or PASS" );
            return false;
        }
        auto client = provision::peers::InfoManager::get();
        if( !client ) {
            return false;
        }
        char* users = client->getUsers();
        char* user_ptr = strstr( users, user );
        if( !user_ptr ) {
            FUNC_LOGI( "USER not found" );
            return false;
        }
        if( ( user_ptr[-1] != '"' ) || ( user_ptr[strlen( user )] != '"' ) ) {
            FUNC_LOGI( "NOT found all name" );
            return false;
        }
        char* rn = strstr( user_ptr, "\r\n" );
        if( !rn ) {
            FUNC_LOGI( "NOT found rn" );
            return false;
        }
        rn[0] = 0;
        uint8_t shaResult[32];

        mbedtls_md_context_t ctx;
        mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
        const size_t payloadLength = strlen( pass );

        mbedtls_md_init( &ctx );
        mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
        mbedtls_md_starts( &ctx );
        mbedtls_md_update( &ctx, (const unsigned char*)pass, payloadLength );
        mbedtls_md_finish( &ctx, shaResult );
        mbedtls_md_free( &ctx );

        constexpr auto bytesCount = sizeof( shaResult );
        constexpr auto stringRepLen = 4;
        constexpr auto bufLen = bytesCount * stringRepLen;
        char pass_sha256[bufLen + 1];
        for( int i = 0; i < bytesCount; ++i ) {
            sprintf( pass_sha256 + i * stringRepLen, "%02x", shaResult[i] );
        }
        pass_sha256[bufLen] = '\0';

        char* pass_ptr = strstr( &user_ptr[strlen( user )], pass_sha256 );
        rn[0] = '\r';
        if( !pass_ptr ) {
            FUNC_LOGI( "BAD PASS" );
            //        return false;
        }
        return true;
    }
};  // struct LoginHandler

}  // namespace

bool LocalNmaClient::initializeImpl() {
    if( !NmaBase::initializeImpl() ) {
        return false;
    }

    setJsonHandler( std::make_unique<LoginHandler>( *this, m_espLogger, m_gatewayUserDataStorage ) );

    return joinToBus();
}

}  // namespace local_nma_srv
