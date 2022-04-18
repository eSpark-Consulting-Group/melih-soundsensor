#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "scenes/commands/SetSceneName.h"

#include <tuple>
#include <list>
#include <chrono>

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesNameSet( const gason::JsonValue& json,
                                                                     common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );

    StringType sceneStorId;
    {
        auto storId = params( tokens::SubId ).toString();
        if( !storId ) {
            return std::make_tuple( errors::ParseError, "\"_id\" field is missing or not json string" );
        }
        sceneStorId = storId;
    }

    StringType newName;
    {
        auto name = params( tokens::Name ).toString();
        if( !name ) {
            return std::make_tuple( errors::ParseError, "\"name\" field is missing or not json string" );
        }
        newName = name;
    }

    auto resp = m_client.asyncSend<scenes::commands::SetSceneName>(
        scenes::commands::LOCAL_BUS_NAME, std::move( sceneStorId ), std::move( newName ) );
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

}  // namespace nma
