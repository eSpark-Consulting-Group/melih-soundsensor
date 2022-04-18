#pragma once

#include "ScenesManagerBase.h"
#include "GlobalScene.h"

namespace scenes {

extern const char SMN_LOG_TAG_GLOB[];

class GlobalScenesManager : public ScenesManagerBase<GlobalScene, GLOBAL_SCENES_PATH, SMN_LOG_TAG_GLOB> {
public:
    using SceneType = GlobalScene;
    using ParentType = ScenesManagerBase<GlobalScene, GLOBAL_SCENES_PATH, SMN_LOG_TAG_GLOB>;

    const char* logTag = SMN_LOG_TAG_GLOB;

    // Use constructors from base class
    using ParentType::ScenesManagerBase;

private:
    bool initializeImpl() override;

    bool isSceneTriggered( const GlobalScene& scene ) const override;

    void onSceneCreated( const SceneType& scene ) override;

    void onSceneDeleted( const SceneType& scene ) override;

    void onSceneChanged( const SceneType& newScene, const bool nameWasChanged ) override;

    void onSceneChanged( const SceneType& newScene, const SceneType& oldScene ) override;

    void sendSceneProgressCmd( const SceneType& scene, const char* state, bool isNotifyUsers = true ) override;

};  // class GlobalScenesManager

}  // namespace scenes
