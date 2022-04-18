#include "GlobalScenesManager.h"

#include "JsonGenerator.h"

#include "scenes/responces/GlobalSceneAdded.h"
#include "scenes/responces/GlobalSceneDeleted.h"
#include "scenes/responces/GlobalSceneChanged.h"
#include "scenes/responces/GlobalSceneRunProgress.h"

namespace scenes {

const char SMN_LOG_TAG_GLOB[] = "[GlobScenMan] ";

bool GlobalScenesManager::initializeImpl() {
    if( !ParentType::initializeImpl() ) {
        SMN_LOGE( "Can't initialize ScenesManagerBase" );
        return false;
    }

    if( !init( commands::GLOBAL_BUS_NAME, getGlobalStorage() ) ) {
        SMN_LOGE( "init() failed" );
        return false;
    }

    SMN_LOGI( "Initialized successfully" );
    return true;
}

void GlobalScenesManager::onSceneCreated( const SceneType& scene ) {
    SMN_LOGI( "Global scene id=%s was created", scene.externStorId().c_str() );

    send<responces::GlobalSceneAdded>( "nma", scene.externStorId() );
}

void GlobalScenesManager::onSceneDeleted( const SceneType& scene ) {
    SMN_LOGI( "Global scene id=%s was deleted", scene.externStorId().c_str() );

    send<responces::GlobalSceneDeleted>( "nma", scene.externStorId() );
}

void GlobalScenesManager::onSceneChanged( const SceneType& newScene, const bool nameWasChanged ) {
    SMN_LOGI( "Scene id=%s was changed, nameWasChanged=%d", newScene.externStorId().c_str(), (int)nameWasChanged );

    send<responces::GlobalSceneChanged>( "nma", newScene.externStorId(), nameWasChanged );
}

void GlobalScenesManager::onSceneChanged( const SceneType& newScene, const SceneType& oldScene ) {
    SMN_LOGI( "Scene id=%s was changed", newScene.externStorId().c_str() );

    send<responces::GlobalSceneChanged>( "nma", newScene.externStorId(), false );
}

void GlobalScenesManager::sendSceneProgressCmd( const SceneType& scene, const char* status, bool isNotifyUsers ) {
    send<responces::GlobalSceneRunProgress>( "nma", scene.externStorId(), status, scene.name(), scene.tag() );
}

bool GlobalScenesManager::isSceneTriggered( const GlobalScene& scene ) const {
    for( const auto& condUuid : scene.conditions() ) {
        if( m_prevTrig.end() == m_prevTrig.find( condUuid ) && m_currTrig.end() != m_currTrig.find( condUuid ) ) {
            if( scene.isEnabled() ) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace scenes
