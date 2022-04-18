#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "Bus.h"

#include "scenes/commands/ScenesList.h"

#include <tuple>
#include <list>
#include <chrono>
#include "DebugUtils.h"

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesListImpl( const char* busName,
                                                                      const gason::JsonValue& json,
                                                                      common::JsonStreamGenerator& gen ) {
    if( !gen.writeOpeningObject() || !gen.writePropertyKey( tokens::Scenes ) ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    bool isSuccess;
    bool isOnlyId = json( tokens::Params )( tokens::OnlyId ).toBool( &isSuccess );

    auto resp = isSuccess ? m_client.asyncSend<scenes::commands::ScenesList>( busName, gen, isOnlyId ) :
                            m_client.asyncSend<scenes::commands::ScenesList>( busName, gen );

    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) -> ResponceType {
        if( !asyncResp.valid() ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }

        auto [errorCode, errorDescr] = asyncResp.get();
        if( errors::NotError != errorCode ) {
            if( errors::GenJsonFailed != errorCode ) {
                gen.writeEmptyArray() && gen.writeClosingSymbol();
            }
            return std::make_tuple( errorCode, std::move( errorDescr ) );
        }

        if( !gen.writeClosingSymbol() ) {
            return std::make_tuple( errors::GenJsonFailed, StringType{} );
        }

        return std::make_tuple( errors::NotError, StringType() );
    };
    m_taskClbk = std::move( clbk );

    return std::make_tuple( errors::Deffered, StringType() );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesList( const gason::JsonValue& json,
                                                                  common::JsonStreamGenerator& gen ) {
    return onHubScenesListImpl( scenes::commands::LOCAL_BUS_NAME, json, gen );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesGlobalList( const gason::JsonValue& json,
                                                                        common::JsonStreamGenerator& gen ) {
    return onHubScenesListImpl( scenes::commands::GLOBAL_BUS_NAME, json, gen );
}

}  // namespace nma
