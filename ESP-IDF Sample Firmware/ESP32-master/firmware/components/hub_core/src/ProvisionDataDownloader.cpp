#include <ProvisionDataDownloader.h>
#include <HTTPClient.h>
#include <TCPSocket.h>
#include <SSLSocket.h>
#include <HTTPData/HTTPText.h>
#include <DebugUtils.h>
#include <IDInfo.h>
#include <Utils.h>
#include <gason.hpp>

namespace core {

provision::SyncRes ProvisionDataDownloader::download( const std::string& addr, ProvisionData& provisionData ) {
    FUNC_LOGI( "Start download provision data from: %s", addr.c_str() );

    auto& idInfo = core::IDInfo::get();
    network::ISocket::UPtr socket;
    if( common::stringStartsWith( addr.c_str(), "https" ) ) {
        auto ssl_socket = std::make_unique<network::SSLSocket>();
        ssl_socket->setCertificates( idInfo.CACertificate(), idInfo.TLSPrivateKey(), idInfo.TLSPublicCertificate() );
        socket = std::move( ssl_socket );
    }
    else {
        socket = std::make_unique<network::TCPSocket>();
    }

    if( !socket ) {
        FUNC_LOGE( "Failed to create socket" );
        return provision::SyncRes::Failed;
    }

    network::HTTPClient httpClient( *socket );
    const auto provisioningToken = idInfo.provisioningServerToken();
    const char* header[] = { AUTH_KEY, provisioningToken.c_str() };
    httpClient.customHeaders( header, 1 );
    std::vector<char> buff( BUFFER_SIZE );
    network::HTTPText reply( buff.data(), buff.size() );
    char req_buffer[] = "\r\n";
    network::HTTPText request( req_buffer );
    FUNC_LOGI( "Try to receive provision data from %s", addr.c_str() );
    if( network::HTTPResult::OK == httpClient.post( addr.c_str(), request, &reply ) ) {
        std::size_t len = std::strlen( buff.data() );
        FUNC_LOGI( "Receive provision data. Size: %d", len );
        if( len == 0 ) {
            FUNC_LOGI( "Provision data are up to date" );
            return provision::SyncRes::NoChanges;
        }
        if( len != 0 && !parseProvisionData( buff.data(), provisionData ) ) {
            FUNC_LOGE( "Failed to parse provision data" );
            return provision::SyncRes::Failed;
        }
    }
    else {
        FUNC_LOGE( "Failed downloading provisioning data from server" );
        return provision::SyncRes::Failed;
    }

    return provision::SyncRes::Updated;
}

bool ProvisionDataDownloader::parseProvisionData( char* data, ProvisionData& provisionData ) {
    std::unique_ptr<gason::JsonAllocator> jsonAlloc = std::make_unique<gason::JsonAllocator>();
    gason::JsonValue json;
    if( !jsonAlloc || gason::JSON_PARSE_OK != gason::jsonParse( data, json, *jsonAlloc ) ) {
        return false;
    }
    bool result;
    ProvisionData tempData;
    auto getString = [&]( const char* name, std::string& value ) -> bool {
        bool result;
        const char* buff = json( name ).toString( &result );
        result = result && buff;
        if( result ) {
            value = buff;
        }
        return result;
    };

    tempData.configRevision = json( "config_version" ).toInt( &result );
    result = result && getString( "provision_server", tempData.provisionServer );
    result = result && getString( "provision_token", tempData.provisionToken );
    result = result && getString( "cloud_server", tempData.cloudServer );
    result = result && getString( "ssl_private_key", tempData.sslPrivateKey );
    result = result && getString( "ssl_shared_key", tempData.sslSharedKey );
    result = result && getString( "signing_ca_certificate", tempData.signingCaCertificate );
    if( !getString( "product_name", tempData.productName ) ) {
        tempData.productName = "";
    }
    if( result ) {
        provisionData = std::move( tempData );
    }

    return result;
}

}  // namespace core
