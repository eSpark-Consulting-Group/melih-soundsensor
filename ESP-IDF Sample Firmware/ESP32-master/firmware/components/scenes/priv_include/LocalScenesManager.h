#pragma once

#include "ScenesManagerBase.h"
#include "LocalScene.h"

namespace scenes {

extern const char SMN_LOG_TAG_REV2[];

class LocalScenesManager : public ScenesManagerBase<LocalScene, LOCAL_SCENES_PATH, SMN_LOG_TAG_REV2> {
public:
    using SceneType = LocalScene;
    using ParentType = ScenesManagerBase<LocalScene, LOCAL_SCENES_PATH, SMN_LOG_TAG_REV2>;

    const char* logTag = SMN_LOG_TAG_REV2;

    LocalScenesManager();

private:
    bool initializeImpl() override;

    bool connectToBus( const char* ownBusName ) override;

    bool isSceneTriggered( const LocalScene& scene ) const override;

    void sendSceneProgressCmd( const SceneType& scene, const char* state, bool isNotifyUsers = true ) override;

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    void updateConditions( const WhenBlock& condition ) override;

    void updateCheckers() override;
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

    void onSceneChanged( const SceneType& newScene, const bool nameWasChanged ) override;

    void onSceneChanged( const SceneType& newScene, const SceneType& oldScene ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::SetSceneName& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::SetSceneHouseModes& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::TimeList& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::EnumerateLocalScenes& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::ScenesData& command ) override;

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const dev_dis::responce::DiscoveredDevices& responce ) override;

private:
    WifiBeaconConditionChecker m_beaconsChecker;
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
};

}  // namespace scenes
