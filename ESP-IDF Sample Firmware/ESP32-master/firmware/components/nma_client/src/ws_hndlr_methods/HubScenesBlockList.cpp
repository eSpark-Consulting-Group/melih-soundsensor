#include "NmaWebSocketHandler.h"
#include "scenes/Utils.h"

namespace nma {

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesBlocksList( const gason::JsonValue& json,
                                                                        common::JsonStreamGenerator& gen ) {
    return scenes::onHubScenesBlocksList( json, gen );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesTriggerDevicesList( common::JsonStreamGenerator& gen ) {
    return scenes::onHubScenesTriggerDevicesList( gen );
}

}  // namespace nma
