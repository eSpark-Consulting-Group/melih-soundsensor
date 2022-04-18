#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DeviceInfo.h"
#include "GatewayUtils.h"

#include <tuple>
#include <set>
#include <DebugUtils.h>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubItemsList( const gason::JsonValue& json,
                                                                 common::JsonStreamGenerator& gen ) {
    std::set<std::string> devices;

    auto jsDevicesIds = json( tokens::Params )( tokens::DeviceIds );
    if( jsDevicesIds ) {
        if( jsDevicesIds.isArray() ) {
            for( const auto item : jsDevicesIds ) {
                const char* cstr = item.value.toString();
                if( !cstr ) {
                    gen.writeOpeningObject() && gen.writeClosingSymbol();
                    return std::make_tuple( errors::InvalidParams, "Device id isn't string!" );
                }
                devices.emplace( cstr );
            }
        }
        else {
            gen.writeOpeningObject() && gen.writeClosingSymbol();
            return std::make_tuple( errors::InvalidParams, "deviceIds value isn't array!" );
        }
    }

    if( !gen.writeOpeningObject() || !gen.writePropertyKey( tokens::Items ) || !gen.writeOpeningArray() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }
    bool isSuccess = true;
    auto clbk = [&isSuccess, &gen, &devices]( const gateways::ItemInfo& itemInfo ) {
        if( !devices.empty() ) {
            if( devices.end() == devices.find( itemInfo.getDeviceId() ) ) {
                return true;
            }
        }
        isSuccess =
            gen.writeOpeningObject() && gateways::utils::writeItemInfo( gen, itemInfo ) && gen.writeClosingSymbol();
        return isSuccess;
    };

    gateways::enumerateItemsFromDb( clbk );
    if( !gen.writeClosingSymbol() || !gen.writeClosingSymbol() || !isSuccess ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errors::NotError, std::string() );
}

}  // namespace nma
