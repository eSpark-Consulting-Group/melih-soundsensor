#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubNetworkEnabledSet( const gason::JsonValue& json,
                                                                         common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
    auto params = json( tokens::Params );
    if( params ) {
        auto interfaceIdVal = params( common::resources::tokens::InterfaceId );
        if( interfaceIdVal.isString() ) {
            const char* interfaceId = interfaceIdVal.toString();
            if( !strcmp( interfaceId, wifi::tokens::WLAN ) ) {
                auto enabledVal = params( common::resources::tokens::Enabled );
                if( enabledVal.isString() ) {
                    if( strcmp( enabledVal.toString(), tokens::Auto ) ) {
                        return std::make_tuple( errors::ApplicationError, errors::data::OperationProhibited );
                    }
                }
            }
            else {
                return std::make_tuple( errors::ApplicationError, errors::data::NetworkInvalidInterface );
            }
        }
        else {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
    }
    else {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma

#endif  // not CONFIG_UART_API
