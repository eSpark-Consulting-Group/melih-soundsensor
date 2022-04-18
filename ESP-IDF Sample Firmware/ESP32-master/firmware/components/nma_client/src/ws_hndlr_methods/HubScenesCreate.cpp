#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "Bus.h"
#include "scenes/commands/CreateScene.h"

#include <tuple>
#include <list>
#include <chrono>

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesCreate( const gason::JsonValue& json,
                                                                    common::JsonStreamGenerator& gen ) {
    auto params = json( tokens::Params );
    auto resp = m_client.asyncSend<scenes::commands::CreateScene>( scenes::commands::LOCAL_BUS_NAME, params );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) -> ResponceType {
        if( std::future_status::ready != asyncResp.wait_for( std::chrono::milliseconds( 0 ) ) ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }

        auto [errorCode, msg] = asyncResp.get();
        if( errors::NotError != errorCode ) {
            gen.writeEmptyObject() && gen.writeItemDelimiter();
            return std::make_tuple( errorCode, std::move( msg ) );
        }

        // if errors::NotError == errorCode than msg contains created scene id
        gen.writeOpeningObject() && gen.writeProperty( tokens::SubId, msg ) && gen.writeClosingSymbol();

        return std::make_tuple( errors::NotError, StringType() );
    };
    m_taskClbk = std::move( clbk );

    return std::make_tuple( errors::Deffered, StringType() );
}

WebSocketHandler::ResponceType WebSocketHandler::onHubGlobalScenesCreate( const gason::JsonValue& json,
                                                                          common::JsonStreamGenerator& gen ) {
    auto params = json( tokens::Params );
    auto resp = m_client.asyncSend<scenes::commands::CreateScene>( scenes::commands::GLOBAL_BUS_NAME, params );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) -> ResponceType {
        if( std::future_status::ready != asyncResp.wait_for( std::chrono::milliseconds( 0 ) ) ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }

        auto [errorCode, msg] = asyncResp.get();
        if( errors::NotError != errorCode ) {
            gen.writeEmptyObject() && gen.writeItemDelimiter();
            return std::make_tuple( errorCode, std::move( msg ) );
        }

        // if errors::NotError == errorCode than msg contains created scene id
        gen.writeOpeningObject() && gen.writeProperty( tokens::SubId, msg ) && gen.writeClosingSymbol();

        return std::make_tuple( errors::NotError, StringType() );
    };
    m_taskClbk = std::move( clbk );

    return std::make_tuple( errors::Deffered, StringType() );
}

}  // namespace nma
