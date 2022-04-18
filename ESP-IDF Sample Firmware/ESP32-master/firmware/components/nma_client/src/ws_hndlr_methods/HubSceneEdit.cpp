#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "scenes/commands/EditScene.h"

#include <tuple>
#include <list>
#include <chrono>

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesEdit( const gason::JsonValue& json,
                                                                  common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        return std::make_tuple( errors::ParseError, "Can't find \"params\" field" );
    }

    auto sceneStorId = params( tokens::SubId ).toString();
    if( !sceneStorId ) {
        return std::make_tuple( errors::ParseError, "Can't find \"_id\" field" );
    }

    auto newSceneJs = params( tokens::Eo );
    if( !newSceneJs || !newSceneJs.isObject() ) {
        return std::make_tuple( errors::ParseError, "Can't find \"eo\" field" );
    }

    auto resp =
        m_client.asyncSend<scenes::commands::EditScene>( scenes::commands::LOCAL_BUS_NAME, sceneStorId, newSceneJs );
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

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesGlobalEdit( const gason::JsonValue& json,
                                                                        common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        return std::make_tuple( errors::ParseError, "Can't find \"params\" field" );
    }

    auto sceneStorId = params( tokens::SubId ).toString();
    if( !sceneStorId ) {
        return std::make_tuple( errors::ParseError, "Can't find \"_id\" field" );
    }

    auto newSceneJs = params( tokens::Scene );
    if( !newSceneJs || !newSceneJs.isObject() ) {
        return std::make_tuple( errors::ParseError, "Can't find \"scene\" field" );
    }

    auto resp =
        m_client.asyncSend<scenes::commands::EditScene>( scenes::commands::GLOBAL_BUS_NAME, sceneStorId, newSceneJs );
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
