#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "settings/commands/BusCommandDeviceSettingValueSet.h"
#include "DeviceInfo.h"
#include "IValue.h"
#include "IValueCast.h"
#include <tuple>
#include "DeviceSettingsInfo.hpp"
#include "SettingStorage.h"

namespace nma {

using namespace common::resources;
using namespace std::string_literals;

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceSettingValueSet( const gason::JsonValue& request,
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

    auto info = ::storage::DeviceSettings::get().getInfo( settingIdJson.toString() );
    if( !info ) {
        return std::make_tuple( errors::ApplicationError, errors::data::ObjectNotFound );
    }

    auto wrongValueTypeErrorMessage = errors::data::EmptyField + "."s + tokens::Value;
    auto value = ::common::makeIval( request( tokens::Params )( tokens::Value ) );

    // Validate if only not an action - action doesn't require a value.
    if( info->getValueType() != ::gateways::ItemValueType::Action ) {
        if( !value ) {
            return std::make_tuple( errors::InvalidParams, wrongValueTypeErrorMessage );
        }

        if( info->isMinMax() ) {
            auto fval = common::iValueCast<float>( value );
            if( std::isnan( fval ) ) {
                return std::make_tuple( errors::InvalidParams, wrongValueTypeErrorMessage );
            }

            const auto minmax = info->getMinMax();
            if( minmax.first > fval || minmax.second < fval ) {
                return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
            }
        }
        else if( info->isEnum() && info->getValueType() != ::gateways::ItemValueType::Array ) {
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
        std::make_unique<settings::commands::DeviceSettingValueSet>( settingIdJson.toString(), std::move( value ) );
    m_client.send( "debug", std::move( cmd ) );

    return std::make_tuple( errors::NotError, "" );
}

}  // namespace nma
