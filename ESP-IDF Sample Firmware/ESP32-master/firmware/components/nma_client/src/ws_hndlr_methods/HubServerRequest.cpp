#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include <BusCommandNmaServerRequest.h>

#include <list>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubServerRequest( const gason::JsonValue& json,
                                                                     common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto cmd = std::make_unique<bus::command::nma::ServerRequest>();
    m_client.send( m_client.addr(), std::move( cmd ) );

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma