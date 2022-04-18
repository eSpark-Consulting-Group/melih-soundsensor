// Developed according with
// https://confluence.mios.com/pages/viewpage.action?spaceKey=ELFD&title=Devices+Detection#DevicesDetection-hub.detection.device.remove

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "dev_dis/DevDiscovery.h"

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDetectionDeviceRemove( const gason::JsonValue& json,
                                                                             common::JsonStreamGenerator& gen ) {
    if( !gen.writeEmptyObject() || !gen.writeItemDelimiter() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    const auto devUuidCStr = params( tokens::SubId ).toString();
    auto devUuid = dev_dis::Uuid::createFromHexStr( devUuidCStr );
    if( !devUuid ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    auto instance = dev_dis::DevDiscovery::get();
    if( !instance ) {
        return std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }

    if( !instance->removeDevice( *devUuid ) ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
