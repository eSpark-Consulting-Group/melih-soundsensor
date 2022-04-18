#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "time_service/Timezone.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubTimeLocationList( const gason::JsonValue& json,
                                                                        common::JsonStreamGenerator& jsGen ) {
    bool result = jsGen.writeOpeningObject();
    {
        result = result && jsGen.writePropertyKey( tokens::Locations );
        result = result && jsGen.writeOpeningArray();
        result = result && jsGen.writeProperty( nullptr, time_service::getTimeZoneId() );
        result = result && jsGen.writeClosingSymbol();
    }
    result = result && jsGen.writeClosingSymbol();
    if( !result ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma
