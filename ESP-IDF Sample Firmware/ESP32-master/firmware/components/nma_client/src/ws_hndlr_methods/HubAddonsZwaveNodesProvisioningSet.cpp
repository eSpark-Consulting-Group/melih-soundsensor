#include <gason.hpp>

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "CommonResources.h"

namespace nma {

WebSocketHandler::ResponceType WebSocketHandler::onHubAddonsZwaveNodesProvisioningSet(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();
    return { ::common::resources::errors::ApplicationError, "The operation is not supported." };
}

}  // namespace nma
