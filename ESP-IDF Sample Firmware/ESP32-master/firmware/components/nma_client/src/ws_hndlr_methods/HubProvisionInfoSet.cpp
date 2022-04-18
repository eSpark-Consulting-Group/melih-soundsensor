#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "IDInfoManager.h"
#include "IDInfo.h"
#include "BusCommandHandleEventUpdateProductName.hpp"
#include "Shutdownable.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubProvisionInfoSet( const gason::JsonValue& json,
                                                                        common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    auto& idInfo = core::IDInfo::get();
    auto params = json( tokens::Params );
    core::ProvisionData prov_data;
    auto getString = [&]( const char* name, std::string& value ) -> bool {
        bool result;
        const char* buff = params( name ).toString( &result );
        result = result && buff;
        if( result ) {
            value = buff;
        }
        return result;
    };
    bool result;
    bool is_updated = false;
    prov_data.configRevision = params( "configRev" ).toInt( &result );
    result = result && getString( "provisionServer", prov_data.provisionServer );
    result = result && getString( "cloudServer", prov_data.cloudServer );
    if( result ) {
        if( !getString( "provisionToken", prov_data.provisionToken ) ) {
            prov_data.provisionToken = idInfo.provisioningServerToken();
        }
        if( !getString( "sslPrivateKey", prov_data.sslPrivateKey ) ) {
            prov_data.sslPrivateKey = idInfo.TLSPrivateKey();
        }
        if( !getString( "sslSharedKey", prov_data.sslSharedKey ) ) {
            prov_data.sslSharedKey = idInfo.TLSPublicCertificate();
        }
        if( !getString( "signingCaCertificate", prov_data.signingCaCertificate ) ) {
            prov_data.signingCaCertificate = idInfo.CACertificate();
        }
        if( !getString( "productName", prov_data.productName ) ) {
            if( idInfo.productName() ) {
                prov_data.productName = idInfo.productName();
            }
            else {
                prov_data.productName = "";
            }
        }

        int new_config_rev;
        is_updated = core::provision::updateProvisionData( prov_data, new_config_rev );

        jsGen.writeOpeningObject();
        {
            jsGen.writeProperty( "configRev", new_config_rev );
            jsGen.writeProperty( "isUpdated", is_updated );
        }
        jsGen.writeClosingSymbol();

        if( is_updated ) {
            m_client.send<bus::command::HandleEventUpdateProductName>(
                core::commands::BUS_NAME, std::string{}, std::string{}, 0 );
        }
    }
    else {
        jsGen.writeNullValue() && jsGen.writeItemDelimiter();
        errorCode = errors::InvalidParams;
    }

    if( is_updated ) {
        m_client.updateProvisionDataRequest();
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma
