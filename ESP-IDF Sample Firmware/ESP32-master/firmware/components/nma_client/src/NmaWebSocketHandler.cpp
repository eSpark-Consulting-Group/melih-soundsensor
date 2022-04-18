#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "RemoteLogger.h"

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::WebSocketHandler( NmaBase& client,
                                    std::shared_ptr<::core::EspLogger> espLogger,
                                    ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage )
: m_client( client )
, m_espLogger( std::move( espLogger ) )
, m_gatewayUserDataStorage( gatewayUserDataStorage ) {}

WebSocketHandler::ResponceType WebSocketHandler::onHubSceneRunProgressBroadcast( const gason::JsonValue& json,
                                                                                 common::JsonStreamGenerator& gen ) {
    FUNC_LOGI( "WebSocketHandler::onHubSceneRunProgressBroadcast" );
    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma
