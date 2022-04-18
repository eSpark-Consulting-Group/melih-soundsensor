#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DebugUtils.h"
#include "HubSettings.h"

namespace nma {

using namespace common::resources;
using namespace core;
namespace {

static bool createAnswerForName( std::string name, common::JsonStreamGenerator& jsonGenerator ) {
    std::string value;
    std::string type;
    std::vector<std::string> params;

    auto tokenAnswer = [&jsonGenerator, name, &value, &params, &type]() {
        bool result = jsonGenerator.writeOpeningObject();
        result = jsonGenerator.writeProperty( tokens::Name, name );
        result = result && jsonGenerator.writeProperty( tokens::ValueType, tokens::Token );

        result = result && jsonGenerator.writePropertyKey( tokens::Enum );
        result = result && jsonGenerator.writeOpeningArray();
        for( const auto param : params ) {
            result = result && jsonGenerator.writeProperty( nullptr, param.c_str() );
        }
        result = result && jsonGenerator.writeClosingSymbol();

        result = result && jsonGenerator.writeProperty( tokens::Value, value );

        result = result && jsonGenerator.writeClosingSymbol();

        return result;
    };

    auto stringAnswer = [&jsonGenerator, name, &value]() {
        bool result = jsonGenerator.writeOpeningObject();
        result = jsonGenerator.writeProperty( tokens::Name, name );
        result = result && jsonGenerator.writeProperty( tokens::ValueType, tokens::String );
        result = result && jsonGenerator.writeProperty( tokens::Value, value );
        result = result && jsonGenerator.writeClosingSymbol();

        return result;
    };

    auto dictAnswer = [&jsonGenerator, name, &value]( std::map<std::string, int> map ) {
        bool result = jsonGenerator.writeOpeningObject();
        result = jsonGenerator.writeProperty( tokens::Name, name );

        for( const auto& item : map ) {
            result = result && jsonGenerator.writeProperty( item.first.c_str(), item.second );
        }

        result = result && jsonGenerator.writeClosingSymbol();

        return result;
    };

    auto boolAnswer = [&jsonGenerator, name, &type]( bool value ) {
        bool result = jsonGenerator.writeOpeningObject();
        result = jsonGenerator.writeProperty( tokens::Name, name );
        result = result && jsonGenerator.writeProperty( tokens::ValueType, tokens::Bool );
        result = result && jsonGenerator.writeProperty( tokens::Value, value );

        result = result && jsonGenerator.writeClosingSymbol();

        return result;
    };

    bool result = false;

    if( name == tokens::ScaleTemperature ) {
        TemperatureScaleEnum val;
        params = HubSettingsInfo::temperatureScaleValues();
        HubSettings::get().getTemperatureScale( val, type );
        value = HubSettingsInfo::temperatureScaleToStr( val );
        result = tokenAnswer();
    }
    else if( name == tokens::TimeFormat ) {
        TimeFormatEnum val;
        params = HubSettingsInfo::timeFormatValues();
        HubSettings::get().getTimeFormat( val, type );
        value = HubSettingsInfo::timeFormatToStr( val );
        result = tokenAnswer();
    }
    else if( name == tokens::DateFormat ) {
        DateFormatEnum val;
        params = HubSettingsInfo::dateFormatValues();
        HubSettings::get().getDateFormat( val, type );
        value = HubSettingsInfo::dateFormatToStr( val );
        result = tokenAnswer();
    }
    else if( name == tokens::LocationUUID ) {
        value = HubSettings::get().getLocationUuid();
        result = stringAnswer();
    }
    else if( name == tokens::FirstStart ) {
        bool val = false;
        HubSettings::get().getFirstStart( val, type );
        result = boolAnswer( val );
    }

    else if( name == tokens::BroadcastsDelay ) {
        const auto val = HubSettings::get().getBroadcastDelays();
        result = dictAnswer( val );
    }

    return result;
}

}  // namespace

WebSocketHandler::ResponceType WebSocketHandler::onHubSettingsListRequest(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& jsonGenerator ) {
    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        FUNC_LOGE( "params field is missing or not json object" );
        return std::make_tuple( errors::ParseError, "\"params\" field is missing or not json object" );
    }

    const auto names = params( tokens::Names );
    std::vector<std::string> namesVector;
    if( names ) {
        if( names.isArray() ) {
            for( auto name : names ) {
                auto str = name.value.toString();
                if( strlen( str ) > 32 )
                    return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
                if( str ) {
                    namesVector.push_back( str );
                }
            }
        }
    }

    bool result = jsonGenerator.writeOpeningObject();
    result&& jsonGenerator.writePropertyKey( tokens::Settings );
    result = result && jsonGenerator.writeOpeningArray();

    if( namesVector.empty() ) {
        namesVector = { tokens::ScaleTemperature, tokens::TimeFormat, tokens::DateFormat,
                        tokens::LocationUUID,     tokens::FirstStart, tokens::BroadcastsDelay };
    }

    for( const auto name : namesVector ) {
        result = createAnswerForName( name, jsonGenerator );
    }

    result = result && jsonGenerator.writeClosingSymbol();

    result = result && jsonGenerator.writeClosingSymbol();

    return result ? std::make_tuple( errors::NotError, "" ) :
                    std::make_tuple( errors::InternalError, errors::data::WrongParams );
}

}  // namespace nma
