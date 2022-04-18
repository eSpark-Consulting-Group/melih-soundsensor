#include "NmaWebSocketHandler.h"

#include "JsonStreamGenerator.h"
#include "CommonResources.h"
#include "DeviceInfo.h"
#include "GatewayUtils.h"

#include <tuple>

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDevicesList( const gason::JsonValue& json,
                                                                   common::JsonStreamGenerator& jsonGenerator ) {
    if( !jsonGenerator.writeOpeningObject() || !jsonGenerator.writePropertyKey( tokens::Devices ) ||
        !jsonGenerator.writeOpeningArray() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    bool isSuccess = true;
    auto clbk = [&isSuccess, &jsonGenerator]( const gateways::BaseDeviceInfo& deviceInfo ) {
        isSuccess = jsonGenerator.writeOpeningObject() &&
                    gateways::utils::writeDeviceInfo( jsonGenerator, deviceInfo ) && jsonGenerator.writeClosingSymbol();
        return isSuccess;
    };
    gateways::enumerateDevicesFromDb( clbk );
    if( !isSuccess || !jsonGenerator.writeClosingSymbol() || !jsonGenerator.writeClosingSymbol() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errors::NotError, StringType{} );
}

}  // namespace nma
