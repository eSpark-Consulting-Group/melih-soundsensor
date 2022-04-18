#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "hub_core/GeoSettings.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubCoordinatesGet( const gason::JsonValue& json,
                                                                      common::JsonStreamGenerator& gen ) {
    auto [latitude, longitude] = core::geo_settings::getCoordinates();
    bool result = gen.writeOpeningObject() && gen.writeProperty( tokens::Latitude, latitude ) &&
                  gen.writeProperty( tokens::Longitude, longitude ) && gen.writeClosingSymbol();
    if( result ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
}

}  // namespace nma