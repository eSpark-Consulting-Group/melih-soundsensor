#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "Bus.h"

#include "scenes/commands/JsonifyScene.h"

#include <tuple>
#include <chrono>

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesGet( const gason::JsonValue& json,
                                                                 common::JsonStreamGenerator& gen ) {
    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        return std::make_tuple( errors::ParseError, "\"params\" field is missing or not json object" );
    }

    auto sceneId = params( tokens::SubId ).toString();
    if( !sceneId ) {
        sceneId = params( tokens::SceneId ).toString();
    }
    if( !sceneId ) {
        return std::make_tuple( errors::ParseError, "Scene id field value is not a json string or missing" );
    }

    auto resp = m_client.asyncSend<scenes::commands::JsonifyScene>( scenes::commands::LOCAL_BUS_NAME, gen, sceneId );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) -> ResponceType {
        if( asyncResp.valid() ) {
            return asyncResp.get();
        }
        return std::make_tuple( errors::Waiting, StringType() );
    };
    m_taskClbk = std::move( clbk );

    return std::make_tuple( errors::Deffered, StringType() );
}

}  // namespace nma
