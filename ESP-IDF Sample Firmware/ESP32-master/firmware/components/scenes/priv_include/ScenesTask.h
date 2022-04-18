#pragma once

#include <memory>

namespace scenes {

class LocalScenesManager;
class ScenesTemplates;
class GlobalScenesManager;

void threadTask( std::shared_ptr<LocalScenesManager> locScenesMan,
#ifdef CONFIG_SPIRAM
                 std::shared_ptr<GlobalScenesManager> globScenesMan,
#endif  // CONFIG_SPIRAM
                 std::shared_ptr<ScenesTemplates> scenesTemplates );

}  // namespace scenes
