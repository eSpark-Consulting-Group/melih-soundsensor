#include "ScenesTask.h"

#include "LocalScenesManager.h"
#include "GlobalScenesManager.h"
#include "templates/HubScenesBlockList.h"

#include "Ezlo.h"
#include "Thread.h"

namespace scenes {

void threadTask( std::shared_ptr<LocalScenesManager> locScenesMan,
#ifdef CONFIG_SPIRAM
                 std::shared_ptr<GlobalScenesManager> globScenesMan,
#endif  // CONFIG_SPIRAM
                 std::shared_ptr<ScenesTemplates> scenesTemplates ) {
    core::upd_srv::UpdContext ctx;
    while( true ) {
        locScenesMan->update( ctx );
#ifdef CONFIG_SPIRAM
        globScenesMan->update( ctx );
#endif  // CONFIG_SPIRAM
        scenesTemplates->update( ctx );

        if( locScenesMan->isShutdowned() &&
#ifdef CONFIG_SPIRAM
            globScenesMan->isShutdowned() &&
#endif  // CONFIG_SPIRAM
            scenesTemplates->isShutdowned() ) {
            break;
        }

        ctx.yield();
    }  // while( true )
}

}  // namespace scenes
