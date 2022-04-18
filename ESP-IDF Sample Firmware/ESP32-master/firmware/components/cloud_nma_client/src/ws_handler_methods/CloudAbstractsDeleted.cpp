#include "NmaJsonHandler.h"

#if defined( CONFIG_CLOUD_DEVICES ) and !defined( CONFIG_UART_API )

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "ColudDevicesManager.h"
#include "DebugUtils.h"

#include <string>

namespace cloud_client {

using namespace common::resources;

NmaJsonHandler::ResponceType NmaJsonHandler::onCloudAbstractsDeleted( const gason::JsonValue& json,
                                                                      common::JsonStreamGenerator& gen ) {
    auto devices = json( tokens::Result )( tokens::Devices );
    for( auto device_node : devices ) {
        std::string id{ device_node.key };
        gateways::cloud::ColudDevicesManager::get().deviceRemoved( id );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace cloud_client

#endif  // defined(CONFIG_CLOUD_DEVICES) and !defined(CONFIG_UART_API)
