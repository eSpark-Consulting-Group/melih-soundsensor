#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDetectionDevicesScanStart( const gason::JsonValue& json,
                                                                                 common::JsonStreamGenerator& jsGen ) {
    if( !jsGen.writeEmptyObject() || !jsGen.writeItemDelimiter() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return m_client.enableDeviceDetection( true ) ?
               std::make_tuple( errors::NotError, StringType() ) :
               std::make_tuple( errors::InternalError, errors::messages::InternalOperationError );
}

}  // namespace nma

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
