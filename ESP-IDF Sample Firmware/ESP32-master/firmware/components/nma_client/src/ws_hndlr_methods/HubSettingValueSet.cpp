#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DebugUtils.h"
#include "BusCommandHubSettingsApply.hpp"

namespace nma {

using namespace common::resources;
using namespace core;

WebSocketHandler::ResponceType WebSocketHandler::onHubSettingValueSetRequest(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& jsonGenerator ) {
    jsonGenerator.writeEmptyObject() && jsonGenerator.writeItemDelimiter();

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        FUNC_LOGE( "params field is missing or not json object" );
        return std::make_tuple( errors::ParseError, "\"params\" field is missing or not json object" );
    }

    bool result = false;
    if( params( tokens::Name ).isString() ) {
        std::unique_ptr<common::IValue> value;

        if( params( tokens::Key ) && params( tokens::Key ).isString() && params( tokens::Value ) &&
            params( tokens::Value ).isNumber() ) {
            auto key = params( tokens::Key ).toString();
            if( key ) {
                auto tblValue = std::make_unique<common::TableValue>();
                tblValue->insert( std::string( params( tokens::Key ).toString() ),
                                  std::make_unique<common::IntValue>( params( tokens::Value ).toNumber() ) );
                value = std::move( tblValue );
            }
        }
        else if( params( tokens::Value ).isString() ) {
            value = std::make_unique<common::StringValue>( params( tokens::Value ).toString() );
        }
        else if( params( tokens::Value ).isBoolean() ) {
            value = std::make_unique<common::BoolValue>( params( tokens::Value ).toBool() );
        }

        if( value ) {
            result = true;
            m_client.send<bus::command::HubSettingsApply>(
                core::commands::BUS_NAME, params( tokens::Name ).toString(), std::move( value ) );
        }
    }
    return result ? std::make_tuple( errors::NotError, "" ) :
                    std::make_tuple( errors::InternalError, errors::data::WrongParams );
}

}  // namespace nma
