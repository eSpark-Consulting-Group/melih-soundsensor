#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "hub_core/GeoSettings.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubCoordinatesSet( const gason::JsonValue& json,
                                                                      common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    bool isSuccess;
    auto latitude = params( tokens::Latitude ).toNumber( &isSuccess );
    if( !isSuccess ) {
        return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
    }
    auto longitude = params( tokens::Longitude ).toNumber( &isSuccess );
    if( !isSuccess ) {
        return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
    }

    return core::geo_settings::saveGeoCoordinates( latitude, longitude ) ?
               std::make_tuple( errors::NotError, StringType() ) :
               std::make_tuple( errors::InternalError, errors::messages::InternalOperationError );
}

}  // namespace nma
