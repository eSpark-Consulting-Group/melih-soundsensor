#include "TimezonePriv.h"
#include "TimezoneService.h"
#include "SntpService.h"

#include "DebugUtils.h"

namespace time_service {

bool initializeComponent() {
    if( !initTimezoneData() ) {
        FUNC_LOGE( "Can't initialize timezone" );
        return false;
    }

    {
        std::chrono::seconds timezoneServiceShutdownDelay( 0 );
        auto timezoneService = std::make_unique<TimezoneService>( timezoneServiceShutdownDelay );
        if( !timezoneService || !timezoneService->initialize() ) {
            FUNC_LOGE( "Can't create TimezoneService" );
            return false;
        }
        core::upd_srv::add( std::move( timezoneService ), "TimezoneService" );
    }

    {
        std::chrono::seconds sntpServiceShutdownDelay( 0 );
        auto sntpService = time_service::SntpService::createSingleton( sntpServiceShutdownDelay );
        if( !sntpService ) {
            FUNC_LOGI( "Can't create SntpService service" );
            return false;
        }

        core::upd_srv::add( sntpService, "SntpService" );
    }

    return true;
}

}  // namespace time_service
