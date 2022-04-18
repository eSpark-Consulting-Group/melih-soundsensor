#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "time_service/Timezone.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubTimeLocationGet( const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    bool result = true;
    result = jsGen.writeOpeningObject();
    result = result && jsGen.writeProperty( tokens::Location, time_service::getTimeZoneId() );
    result = result && jsGen.writeClosingSymbol();

    if( !result ) {
        errorCode = errors::ApplicationError;
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma