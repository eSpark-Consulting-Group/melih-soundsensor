#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "settings/commands/BusCommandDeviceSettingValueSet.h"
#include "DeviceInfo.h"
#include "IValueCast.h"
#include <tuple>
#include <settings/commands/BusCommandDeviceSettingReset.h>
#include <DebugUtils.h>
#include "DeviceSettingsInfo.hpp"
#include "SettingStorage.h"

namespace nma {

using namespace common::resources;
using namespace std::string_literals;

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceSettingReset( const gason::JsonValue& json,
                                                                          common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );

    std::string settingId;
    std::string deviceId;

    if( auto settingIdJson = params( tokens::SubId ) ) {
        if( !settingIdJson.isString() ) {
            return { errors::InvalidParams, errors::data::WrongType + "."s + tokens::SubId };
        }
        settingId = settingIdJson.toString();
        if( !::storage::DeviceSettings::get().getInfo( settingId ) ) {
            return std::make_tuple( errors::ApplicationError, errors::data::ObjectNotFound );
        }
    }
    else if( auto deviceIdJson = params( tokens::DeviceId ) ) {
        if( !deviceIdJson.isString() ) {
            return { errors::InvalidParams, errors::data::WrongType + "."s + tokens::DeviceId };
        }
        deviceId = deviceIdJson.toString();
        if( !gateways::BaseDeviceInfo::exists( deviceId ) ) {
            return std::make_tuple( errors::ApplicationError, errors::data::ObjectNotFound );
        }
    }

    m_client.send(
        "debug",
        std::make_unique<settings::commands::DeviceSettingReset>( std::move( settingId ), std::move( deviceId ) ) );
    return std::make_tuple( errors::NotError, "" );
}

}  // namespace nma
