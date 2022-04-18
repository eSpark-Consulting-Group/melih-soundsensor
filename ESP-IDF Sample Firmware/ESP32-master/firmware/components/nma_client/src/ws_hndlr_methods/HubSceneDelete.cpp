#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "Bus.h"

#include "scenes/commands/DeleteScene.h"

#include <tuple>
#include <list>
#include <chrono>

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubSceneDeleteImpl( const char* busName,
                                                                       const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& gen ) {
    // Write empty object result
    if( !gen.writeEmptyObject() || !gen.writeItemDelimiter() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }
    auto sceneId = json( tokens::Params )( tokens::SubId ).toString();
    if( !sceneId ) {
        return std::make_tuple( errors::ParseError, "\"_id\" field is missing or not json string" );
    }

    auto resp = m_client.asyncSend<scenes::commands::DeleteScene>( busName, std::move( sceneId ) );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) -> ResponceType {
        if( std::future_status::ready != asyncResp.wait_for( std::chrono::milliseconds( 0 ) ) ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }
        return asyncResp.get();
    };
    m_taskClbk = std::move( clbk );

    return std::make_tuple( errors::Deffered, StringType() );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubSceneDelete( const gason::JsonValue& json,
                                                                   common::JsonStreamGenerator& gen ) {
    return onHubSceneDeleteImpl( scenes::commands::LOCAL_BUS_NAME, json, gen );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubSceneGlobalDelete( const gason::JsonValue& json,
                                                                         common::JsonStreamGenerator& gen ) {
    return onHubSceneDeleteImpl( scenes::commands::GLOBAL_BUS_NAME, json, gen );
}

}  // namespace nma
