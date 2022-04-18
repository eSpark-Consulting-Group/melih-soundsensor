#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "time_service/commands/SetTimezone.h"
#include "Ezlo.h"

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onCloudTimeZonesGet( const gason::JsonValue& json,
                                                                      common::JsonStreamGenerator& gen ) {
    auto error = json( tokens::Error )( tokens::Data ).toString();
    if( error ) {
        FUNC_LOGE( "TimeZones update failed, %s", error );
        return std::make_tuple( errors::NotError, StringType() );
    }

    auto result = json( tokens::Result );
    if( !result ) {
        return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
    }

    const auto location = result( tokens::Timezone ).toString();
    const auto timeEnvVar = result( tokens::TimeEnvVariable ).toString();
    bool isExpireTime;
    const time_t expireTime = result( tokens::ExpireTime ).toInt( &isExpireTime );

    if( 4 > strlen( location ) || 4 > strlen( timeEnvVar ) ) {
        FUNC_LOGE( "TimeZones update failed" );
        return std::make_tuple( errors::InvalidParams, errors::messages::BadParams );
    }

    m_client.send<time_service::command::SetTimezone>( time_service::command::BUS_NAME,
                                                       location,
                                                       timeEnvVar,
                                                       isExpireTime ? std::make_optional( expireTime ) : std::nullopt );
    FUNC_LOGI( "Timezones updated: location=%s, tzEnvVariable=%s", location, timeEnvVar );

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma