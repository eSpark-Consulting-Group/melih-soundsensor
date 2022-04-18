#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "syscore/commands/Shutdown.h"

namespace nma {

using namespace common::resources;

constexpr auto CmdTimeoutSec = std::chrono::seconds( 3 );

WebSocketHandler::ResponceType WebSocketHandler::onHubReboot( const gason::JsonValue& json,
                                                              common::JsonStreamGenerator& jsGen ) {
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    m_client.send<core::commands::Shutdown>( core::commands::BUS_NAME );

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma
