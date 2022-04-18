#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "Bus.h"

#include "scenes/commands/RunScene.h"

#include <tuple>
#include <chrono>

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesRunImpl( const char* busName,
                                                                     const gason::JsonValue& json,
                                                                     common::JsonStreamGenerator& gen ) {
    // Write empty object result
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        return std::make_tuple( errors::ParseError, "\"params\" field is missing or not json object" );
    }

    auto sceneId = params( tokens::SubId ).toString();
    if( !sceneId ) {
        sceneId = params( tokens::SceneId ).toString();
    }
    if( !sceneId ) {
        return std::make_tuple( errors::ParseError, "Scene id field value is not a json string" );
    }

    auto resp = m_client.asyncSend<scenes::commands::RunScene>( busName, sceneId );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) -> ResponceType {
        if( !asyncResp.valid() ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }
        return asyncResp.get();
    };
    m_taskClbk = std::move( clbk );

    return std::make_tuple( errors::Deffered, StringType() );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesRun( const gason::JsonValue& json,
                                                                 common::JsonStreamGenerator& gen ) {
    return onHubScenesRunImpl( scenes::commands::LOCAL_BUS_NAME, json, gen );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesGlobalRun( const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& gen ) {
    return onHubScenesRunImpl( scenes::commands::GLOBAL_BUS_NAME, json, gen );
}

}  // namespace nma
