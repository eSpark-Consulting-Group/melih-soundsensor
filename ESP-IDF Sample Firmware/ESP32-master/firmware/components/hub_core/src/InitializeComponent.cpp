#include "hub_core/InitializeComponent.h"

#include "UpdatableService.h"
#include "PagesManager.h"
#include "Ezlo.h"
#include "hub_core/GeoSettings.h"

#include "DebugUtils.h"

namespace core {

bool initializeComponent() {
    {
        std::chrono::seconds pagesManagerShutdownDelay( 0 );
        auto pagesManager = core::PagesManager::createSingleton( std::move( pagesManagerShutdownDelay ) );
        if( !pagesManager ) {
            FUNC_LOGE( "Can't create PagesManager singleton!" );
            return false;
        }
        upd_srv::add( std::move( pagesManager ), "PagesManager" );
    }

    {
        std::chrono::seconds ezloShutdownDelay( 3 );
        auto ezlo = core::Ezlo::createSingleton( std::move( ezloShutdownDelay ) );
        if( !ezlo ) {
            FUNC_LOGE( "Can't create Ezlo singleton!" );
            return false;
        }
        upd_srv::add( std::move( ezlo ), "Ezlo" );
    }

    if( !geo_settings::initialize() ) {
        FUNC_LOGE( "Can't initialize geo_settings" );
        return false;
    }

    return true;
}

}  // namespace core
