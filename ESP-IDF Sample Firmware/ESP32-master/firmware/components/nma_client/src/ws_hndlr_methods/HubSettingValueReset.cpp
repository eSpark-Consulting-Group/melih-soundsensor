#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "HubSettings.h"

namespace nma {

using namespace common::resources;
using namespace core;

WebSocketHandler::ResponceType WebSocketHandler::onHubSettingValueResetRequest(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& jsonGenerator ) {
    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        FUNC_LOGE( "params field is missing or not json object" );
        return std::make_tuple( errors::ParseError, "\"params\" field is missing or not json object" );
    }

    bool result = jsonGenerator.writeOpeningObject();

    if( params( tokens::Name ).isString() ) {
        const auto name = params( tokens::Name ).toString();
        if( !strcmp( name, tokens::ScaleTemperature ) ) {
            HubSettings::get().resetTemperatureScale();
        }
        if( !strcmp( name, tokens::TimeFormat ) ) {
            HubSettings::get().resetTimeFormat();
        }
        if( !strcmp( name, tokens::DateFormat ) ) {
            HubSettings::get().resetDateFormat();
        }
        if( !strcmp( name, tokens::FirstStart ) ) {
            HubSettings::get().resetFirstStart();
        }
        if( !strcmp( name, tokens::BroadcastsDelay ) ) {
            HubSettings::get().resetBroadcastDelay();
        }
        else {
            result = result && jsonGenerator.writeClosingSymbol();
            return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
        }
    }
    else {
        result = false;
    }

    result = result && jsonGenerator.writeClosingSymbol();

    return result ? std::make_tuple( errors::NotError, "" ) :
                    std::make_tuple( errors::InternalError, errors::data::WrongParams );
}

}  // namespace nma
