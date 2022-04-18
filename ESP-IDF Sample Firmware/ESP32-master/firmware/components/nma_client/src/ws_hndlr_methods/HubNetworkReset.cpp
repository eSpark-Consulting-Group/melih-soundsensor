#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "wifi_service2/Common.h"
#include "hub_core/Common.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubNetworkReset( const gason::JsonValue& json,
                                                                    common::JsonStreamGenerator& jsGen ) {
    if( !jsGen.writeEmptyObject() || !jsGen.writeItemDelimiter() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    constexpr bool isErasePreviousApCreds = true;
    const bool isSuccess = network::wifi_srv2::disjoinFromAccessPoint( core::hubName(), isErasePreviousApCreds );
    return isSuccess ? std::make_tuple( errors::NotError, StringType() ) :
                       std::make_tuple( errors::InternalError, errors::messages::InternalOperationError );
}

}  // namespace nma

#endif  // not CONFIG_UART_API
