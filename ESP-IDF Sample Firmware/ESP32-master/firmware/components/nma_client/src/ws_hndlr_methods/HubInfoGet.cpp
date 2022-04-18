#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "FirmwareVersion.h"
#include "time_service/Timezone.h"
#include "hub_core/GeoSettings.h"
#include "hub_core/Common.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubInfoGet( const gason::JsonValue& json,
                                                               common::JsonStreamGenerator& gen ) {
    bool isSuccess = gen.writeOpeningObject();
    {
        isSuccess = isSuccess && gen.writeProperty( tokens::Model, MODEL );
        isSuccess = isSuccess && gen.writeProperty( tokens::Architecture, HARDWARE_ARCH );
        isSuccess = isSuccess && gen.writeProperty( tokens::Firmware, FIRMWARE_VERSION );
        isSuccess = isSuccess && gen.writeProperty( tokens::Kernel, KERNEL_VERSION );
        isSuccess = isSuccess && gen.writeProperty( tokens::Hardware, HARDWARE_VERSION );
        isSuccess = isSuccess && gen.writeProperty( tokens::Serial, core::serial() );
        isSuccess = isSuccess && gen.writePropertyKey( tokens::Build );

        isSuccess = isSuccess && gen.writeOpeningObject();
        {
            isSuccess = isSuccess && gen.writeProperty( tokens::Time, BUILD_TIME );
            isSuccess = isSuccess && gen.writeProperty( tokens::Builder, BUILD_BUILDER );
            isSuccess = isSuccess && gen.writeProperty( tokens::Branch, BUILD_BRANCH );
            isSuccess = isSuccess && gen.writeProperty( tokens::Commit, BUILD_COMMIT );
        }
        isSuccess = isSuccess && gen.writeClosingSymbol();

        int64_t n = esp_timer_get_time() / 1000000;  // sec
        int d = n / ( 24 * 3600 );
        n %= ( 24 * 3600 );
        int h = n / 3600;
        n %= 3600;
        int m = n / 60;
        n %= 60;
        char upt_buff[128] = {};
        snprintf( upt_buff, sizeof( upt_buff ), "%dd %dh %dm %ds", d, h, m, (int)n );
        isSuccess = isSuccess && gen.writeProperty( tokens::Uptime, upt_buff );
        time_t t = time( nullptr );

        struct tm* lt = localtime( &t );
        char lt_buff[32] = {};
        strftime( lt_buff, sizeof( lt_buff ), "%FT%T%z", lt );
        isSuccess = isSuccess && gen.writeProperty( tokens::Localtime, lt_buff );

        isSuccess = isSuccess && gen.writePropertyKey( tokens::Location );
        isSuccess = isSuccess && gen.writeOpeningObject();
        {
            auto [latitude, longitude] = core::geo_settings::getCoordinates();
            isSuccess = isSuccess && gen.writeProperty( tokens::Latitude, latitude );
            isSuccess = isSuccess && gen.writeProperty( tokens::Longitude, longitude );
            isSuccess = isSuccess && gen.writeProperty( tokens::Timezone, time_service::getTimeZoneId() );

            if( core::geo_settings::isDefaultCoords() ) {
                if( time_service::isDefaultTimezone() ) {
                    isSuccess = isSuccess && gen.writeProperty( tokens::State, "default" );
                }
                else {
                    isSuccess = isSuccess && gen.writeProperty( tokens::State, "customTimezone" );
                }
            }
            else if( time_service::isDefaultTimezone() ) {
                isSuccess = isSuccess && gen.writeProperty( tokens::State, "customCoordinates" );
            }
            else {
                isSuccess = isSuccess && gen.writeProperty( tokens::State, "customAll" );
            }
        }
        isSuccess = isSuccess && gen.writeClosingSymbol();
    }
    isSuccess = isSuccess && gen.writeClosingSymbol();

    if( isSuccess ) {
        return std::make_tuple( errors::NotError, StringType() );
    }

    return std::make_tuple( errors::ApplicationError, StringType() );
}

}  // namespace nma