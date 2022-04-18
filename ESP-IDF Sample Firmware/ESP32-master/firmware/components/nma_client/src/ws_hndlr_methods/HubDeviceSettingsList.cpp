#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DeviceInfo.h"
#include "GatewayUtils.h"
#include "DeviceSettingsInfo.hpp"

#include <tuple>
#include <set>
#include "SettingStorage.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceSettingsList( const gason::JsonValue& json,
                                                                          common::JsonStreamGenerator& gen ) {
    std::set<std::string> devices;

    auto jsDevicesIds = json( tokens::Params )( tokens::DeviceIds );
    if( jsDevicesIds ) {
        if( jsDevicesIds.isArray() ) {
            for( const auto deviceId : jsDevicesIds ) {
                const char* cstr = deviceId.value.toString();
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

    if( !gen.writeOpeningObject() || !gen.writePropertyKey( tokens::Settings ) || !gen.writeOpeningArray() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }
    bool isSuccess = true;
    auto clbk = [&isSuccess, &gen, &devices]( const gateways::DeviceSettingsInfo& settingInfo ) {
        if( !devices.empty() ) {
            if( devices.end() == devices.find( settingInfo.getItemId() ) ) {
                return true;
            }
        }
        isSuccess = gen.writeOpeningObject() && gateways::utils::writeSettingInfo( gen, settingInfo ) &&
                    gen.writeClosingSymbol();
        return isSuccess;
    };

    ::storage::DeviceSettings::get().enumerate( clbk );

    if( !gen.writeClosingSymbol() || !gen.writeClosingSymbol() || !isSuccess ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }
    return std::make_tuple( errors::NotError, std::string() );
}

}  // namespace nma
