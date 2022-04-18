#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "settings/commands/BusCommandGatewaySettingValueSet.h"
#include "GatewaySettingsInfo.hpp"
#include "SettingStorage.h"
#include "IValue.h"
#include "IValueCast.h"
#include <tuple>

namespace nma {

using namespace common::resources;
using namespace std::string_literals;

WebSocketHandler::ResponceType WebSocketHandler::onHubGatewaySettingValueSet( const gason::JsonValue& request,
                                                                              common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto settingIdJson = request( tokens::Params )( tokens::SubId );

    if( !settingIdJson ) {
        return std::make_tuple( errors::InvalidParams, errors::data::IdNotExist );
    }

    if( !settingIdJson.isString() ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongType + "."s + tokens::SubId );
    }

    if( 0 == std::strlen( settingIdJson.toString() ) ) {
        return std::make_tuple( errors::InvalidParams, errors::data::IdIsEmpty );
    }

    auto info = ::storage::GatewaySettings::get().getInfo( settingIdJson.toString() );
    if( !info ) {
        return std::make_tuple( errors::ApplicationError, errors::data::ObjectNotFound );
    }

    auto wrongValueTypeErrorMessage = errors::data::EmptyField + "."s + tokens::Value;
    auto value = ::common::makeIval( request( tokens::Params )( tokens::Value ) );

    if( info->getValueType() != ::gateways::ItemValueType::Action ) {
        if( !value ) {
            return std::make_tuple( errors::InvalidParams, wrongValueTypeErrorMessage );
        }

        if( !info->castToItemType( value->clone() ) ) {
            return std::make_tuple( errors::InvalidParams, wrongValueTypeErrorMessage );
        }

        if( info->isEnum() ) {
            auto strIval = common::iValueCast<common::StringValue>( value.get() );
            if( !strIval ) {
                return std::make_tuple( errors::InvalidParams, wrongValueTypeErrorMessage );
            }
            if( !info->checkEnumValue( strIval->get() ) ) {
                return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
            }
        }
    }

    auto cmd =
        std::make_unique<settings::commands::GatewaySettingValueSet>( settingIdJson.toString(), std::move( value ) );
    m_client.send( "debug", std::move( cmd ) );

    return std::make_tuple( errors::NotError, "" );
}

}  // namespace nma
