#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "BusCommandPluginRun.h"
#include "Utils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubExtensionsPluginRun( const gason::JsonValue& json,
                                                                           common::JsonStreamGenerator& gen ) {
    if( !gen.writeEmptyObject() || !gen.writeItemDelimiter() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    auto params = json( tokens::Params );
    if( !params ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    auto scriptName = params( tokens::Script ).toString();
    auto scriptCode = params( tokens::Code ).toString();
    if( !scriptName && !scriptCode ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    using StringType = bus::command::PluginRun::StringType;
    m_client.send<bus::command::PluginRun>( "debug",
                                            ( scriptName ? scriptName : StringType() ),
                                            ( scriptCode ? scriptCode : StringType() ),
                                            common::makeIval( params( "scriptParams" ) ) );
    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma
