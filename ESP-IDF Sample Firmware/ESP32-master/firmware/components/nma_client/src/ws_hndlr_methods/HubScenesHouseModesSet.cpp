#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "scenes/commands/SetSceneHouseModes.h"

#include <tuple>
#include <chrono>

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesHouseModesSet( const gason::JsonValue& json,
                                                                           common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        return std::make_tuple( errors::ParseError, "\"params\" field is missing or not json object" );
    }

    StringType sceneStorId;
    {
        auto storId = params( tokens::SubId ).toString();
        if( !storId ) {
            return std::make_tuple( errors::ParseError, "\"_id\" field is missing or not json string" );
        }
        sceneStorId = storId;
    }

    scenes::commands::SetSceneHouseModes::HouseModesList modesList;
    {
        auto modesJsArray = params( tokens::HouseModes );
        if( !modesJsArray || !modesJsArray.isArray() ) {
            return std::make_tuple( errors::ParseError, "\"houseModes\" field is missing or not json array" );
        }
        StringType mode;

        auto jsItr = gason::begin( modesJsArray );
        while( jsItr.isValid() ) {
            auto modeCStr = jsItr->value.toString();
            if( !modeCStr ) {
                return std::make_tuple( errors::ParseError, "array value not json string" );
            }
            modesList.emplace_back( modeCStr );
            ++jsItr;
        }
    }

    auto resp = m_client.asyncSend<scenes::commands::SetSceneHouseModes>(
        scenes::commands::LOCAL_BUS_NAME, std::move( sceneStorId ), std::move( modesList ) );
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
