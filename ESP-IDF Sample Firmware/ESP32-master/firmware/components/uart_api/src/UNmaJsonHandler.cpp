#include "UNmaJsonHandler.h"

#include "LocalUartSrv.h"
#include "CommonResources.h"

namespace ucloud_client {

using namespace common::resources;

UNmaJsonHandler::ResponceType UNmaJsonHandler::onRegistered( const gason::JsonValue& json,
                                                             common::JsonStreamGenerator& gen ) {
    auto res = nma::WebSocketHandler::onRegistered( json, gen );

    if( errors::NotError == std::get<0>( res ) ) {
        auto& client = static_cast<UCloudNmaClient&>( m_client );
        client.onRegistered();
    }

    return std::move( res );
}

}  // namespace ucloud_client