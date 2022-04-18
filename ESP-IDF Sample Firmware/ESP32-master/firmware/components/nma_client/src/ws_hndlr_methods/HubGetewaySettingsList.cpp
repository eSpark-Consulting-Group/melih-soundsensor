#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "GatewaySettingsInfo.hpp"
#include "GatewayUtils.h"
#include "SettingStorage.h"

#include <tuple>
#include <set>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubGatewaySettingsList( const gason::JsonValue& json,
                                                                           common::JsonStreamGenerator& gen ) {
    std::set<std::string> geteways;

    auto jsGatewaysIds = json( tokens::Params )( tokens::GatewayIds );
    if( jsGatewaysIds ) {
        if( jsGatewaysIds.isArray() ) {
            for( const auto getewayId : jsGatewaysIds ) {
                const char* cstr = getewayId.value.toString();
                if( !cstr ) {
                    gen.writeOpeningObject() && gen.writeClosingSymbol();
                    return std::make_tuple( errors::InvalidParams, "Gateway id isn't string!" );
                }
                geteways.emplace( cstr );
            }
        }
        else {
            gen.writeOpeningObject() && gen.writeClosingSymbol();
            return std::make_tuple( errors::InvalidParams, "getewayIds value isn't array!" );
        }
    }

    if( !gen.writeOpeningObject() || !gen.writePropertyKey( tokens::Settings ) || !gen.writeOpeningArray() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }
    bool isSuccess = true;
    auto clbk = [&isSuccess, &gen, &geteways]( const gateways::GatewaySettingsInfo& settingInfo ) {
        if( !geteways.empty() ) {
            if( geteways.end() == geteways.find( settingInfo.getItemId() ) ) {
                return true;
            }
        }
        isSuccess = gen.writeOpeningObject() && gateways::utils::writeGatewaySettingsInfo( gen, settingInfo ) &&
                    gen.writeClosingSymbol();
        return isSuccess;
    };

    ::storage::GatewaySettings::get().enumerate( clbk );

    if( !gen.writeClosingSymbol() || !gen.writeClosingSymbol() || !isSuccess ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }
    return std::make_tuple( errors::NotError, std::string() );
}

}  // namespace nma
