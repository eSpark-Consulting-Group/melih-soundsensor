#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "Bus.h"

#include "scenes/commands/TimeList.h"

#include <tuple>
#include <chrono>

namespace nma {

using namespace common::resources;
using StringType = std::string;

WebSocketHandler::ResponceType WebSocketHandler::onHubScenesTimeList( const gason::JsonValue& json,
                                                                      common::JsonStreamGenerator& gen ) {
    if( !gen.writeOpeningObject() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    auto resp = m_client.asyncSend<scenes::commands::TimeList>( scenes::commands::LOCAL_BUS_NAME, gen );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) -> ResponceType {
        if( !asyncResp.valid() ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }
        auto res = asyncResp.get();
        if( errors::NotError != std::get<0>( res ) ) {
            return std::move( res );
        }

        if( !gen.writeClosingSymbol() ) {
            return std::make_tuple( errors::GenJsonFailed, StringType{} );
        }

        return std::make_tuple( errors::NotError, StringType() );
    };
    m_taskClbk = std::move( clbk );

    return std::make_tuple( errors::Deffered, StringType() );
}

}  // namespace nma
