#include "NmaWebSocketHandler.h"

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "settings/commands/BusCommandDeviceSettingDictionaryValueSet.h"
#include "DeviceInfo.h"
#include "IValueCast.h"
#include <tuple>
#include "DebugUtils.h"
#include "DeviceSettingsInfo.hpp"
#include "SettingStorage.h"

namespace nma {

using namespace common::resources;
using namespace std::string_literals;

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceSettingDictionaryValueSet(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );

    const auto settingId = params( tokens::SubId ).toString();
    if( !settingId ) {
        return std::make_tuple( errors::InvalidParams, errors::data::EmptyField + "."s + tokens::SubId );
    }

    const auto key = params( tokens::Key ).toString();
    if( !key ) {
        return std::make_tuple( errors::InvalidParams, errors::data::EmptyField + "."s + tokens::Key );
    }

    std::unique_ptr<::common::IValue> value;
    if( auto valueJson = params( tokens::Value ) ) {
        value = common::makeIval( valueJson );
        if( !value ) {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongType );
        }
        auto info = ::storage::DeviceSettings::get().getInfo( settingId );
        if( !info ) {
            return std::make_tuple( errors::ApplicationError, errors::data::ObjectNotFound );
        }
        auto newVal = info->castToItemType( value->clone() );
        if( !newVal ) {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongType );
        }
    }

    auto cmd =
        std::make_unique<settings::commands::DeviceSettingDictionaryValueSet>( settingId, std::move( value ), key );
    m_client.send( "debug", std::move( cmd ) );

    return std::make_tuple( errors::NotError, "" );
}

}  // namespace nma
