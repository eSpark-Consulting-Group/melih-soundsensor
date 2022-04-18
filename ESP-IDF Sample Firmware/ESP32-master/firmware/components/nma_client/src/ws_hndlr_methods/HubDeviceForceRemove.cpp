#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "BusCommandCloudForceRemove.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceForceRemove( const gason::JsonValue& json,
                                                                         common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto deviceId = json( tokens::Params )( tokens::SubId ).toString();
    if( !deviceId ) {
        return std::make_tuple( errors::ApplicationError, "The device id is wrong" );
    }

    auto command = std::make_unique<bus::command::CloudForceRemove>();
    command->deviceId = deviceId;
    m_client.send( "debug", std::move( command ) );

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma