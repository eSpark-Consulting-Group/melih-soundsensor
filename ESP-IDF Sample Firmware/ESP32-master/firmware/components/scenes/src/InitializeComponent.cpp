#include "sdkconfig.h"

#include "scenes/Utils.h"
#include "templates/HubScenesBlockList.h"
#include "LocalScenesManager.h"
#include "ScenesTask.h"
#include "Thread.h"

#ifdef CONFIG_SPIRAM
#include "GlobalScenesManager.h"
#else  // not CONFIG_SPIRAM
#include "UpdatableService.h"
#endif  // CONFIG_SPIRAM

#include "DebugUtils.h"

namespace scenes {

bool initializeComponent( std::list<core::Thread>& threadsList ) {
    auto scenesTemplates = std::make_shared<ScenesTemplates>();
    if( !scenesTemplates || !scenesTemplates->initialize() ) {
        FUNC_LOGE( "Can't initialize ScenesTemplates!" );
    }
#ifdef CONFIG_SPIRAM
    auto localScenesManager = std::make_shared<LocalScenesManager>();
    if( !localScenesManager || !localScenesManager->initialize() ) {
        FUNC_LOGE( "Can't init scenes::ScenesManager" );
        return false;
    }

    auto globalScenesManager = std::make_shared<GlobalScenesManager>();
    if( !globalScenesManager || !globalScenesManager->initialize() ) {
        FUNC_LOGE( "Can't init scenes::GlobalScenesManager" );
        return false;
    }

    core::Thread scenesThread(
        "SCENES_TASK", 8 * 1024, threadTask, localScenesManager, globalScenesManager, scenesTemplates );
    if( !scenesThread ) {
        FUNC_LOGE( "Can't run scenes::threadTask!" );
        return false;
    }
    threadsList.push_back( std::move( scenesThread ) );
#else   // not CONFIG_SPIRAM
    auto localScenesManager = std::make_shared<LocalScenesManager>();
    if( !localScenesManager || !localScenesManager->initialize() ) {
        FUNC_LOGE( "Can't init scenes::LocalScenesManager" );
    }
    core::upd_srv::add( std::move( localScenesManager ), "LocalScenesManager" );
    core::upd_srv::add( std::move( scenesTemplates ), "ScenesTemplates" );
#endif  // CONFIG_SPIRAM
    return true;
}

}  // namespace scenes
