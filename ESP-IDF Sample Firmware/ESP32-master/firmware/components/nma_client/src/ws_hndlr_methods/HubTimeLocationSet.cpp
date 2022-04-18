#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "time_service/commands/SetTimezone.h"
#include "BusCommandNmaRequestTimeZones.h"

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;
using namespace std::chrono_literals;

WebSocketHandler::ResponceType WebSocketHandler::onHubTimeLocationSet( const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& jsGen ) {
    auto params = json( tokens::Params );
    if( !params ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
    }

    std::string location, timeEnvVar;
    {
        const auto locationCStr = params( tokens::Location ).toString();
        if( locationCStr && 4 < strlen( locationCStr ) ) {
            location = locationCStr;
        }
        else {
            jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
            return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
        }

        const auto timeEnvVarCStr = params( tokens::TimeEnvVariable ).toString();
        if( timeEnvVarCStr ) {
            if( 4 > strlen( timeEnvVarCStr ) ) {
                jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
                return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
            }

            timeEnvVar = timeEnvVarCStr;
        }
    }

    if( timeEnvVar.empty() ) {
        m_client.send<bus::command::nma::RequestTimeZones>( "nma", location );
    }
    else {
        m_client.send<time_service::command::SetTimezone>( time_service::command::BUS_NAME, location, timeEnvVar );
    }

    auto isSuccess = jsGen.writeOpeningObject();
    isSuccess = isSuccess && jsGen.writeProperty( tokens::Location, location );
    isSuccess = isSuccess && jsGen.writeClosingSymbol();
    return isSuccess ? std::make_tuple( errors::NotError, StringType() ) :
                       std::make_tuple( errors::GenJsonFailed, StringType{} );
}

}  // namespace nma
