#include "LocalScenesManager.h"
#include "LocalScene.h"
#include "scenes/commands/EnumerateLocalScenes.h"
#include "scenes/commands/ScenesData.h"

namespace scenes {

const char SMN_LOG_TAG_REV2[] = "[LoclScenMan] ";

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
LocalScenesManager::LocalScenesManager()
: m_beaconsChecker( m_prevTrig, m_currTrig ) {}
#else   // not CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
LocalScenesManager::LocalScenesManager() {}
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

bool LocalScenesManager::initializeImpl() {
    if( !ParentType::initializeImpl() ) {
        SMN_LOGE( "Can't initialize ScenesManagerBase" );
        return false;
    }

    if( !init( commands::LOCAL_BUS_NAME, getLocalStorage() ) ) {
        SMN_LOGE( "init() failed" );
        return false;
    }

    SMN_LOGI( "Initialized successfully" );
    return true;
}

bool LocalScenesManager::connectToBus( const char* ownBusName ) {
    if( !ParentType::connectToBus( ownBusName ) ) {
        return false;
    }

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    if( !bus::Bus::get().join( dev_dis::responce::BUS_NAME, *this ) ) {
        SMN_LOGE( "Wasn't connected to \"%s\"!", dev_dis::responce::BUS_NAME );
        return false;
    }
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

    return true;
}

bool LocalScenesManager::isSceneTriggered( const LocalScene& scene ) const {
    for( const auto& condUuid : scene.conditions() ) {
        if( m_prevTrig.end() == m_prevTrig.find( condUuid ) && m_currTrig.end() != m_currTrig.find( condUuid ) ) {
            const auto mode = core::HouseModesManager::get().currentPreset();
            if( scene.isEnabled() && scene.checkHouseMode( mode ) ) {
                return true;
            }
        }
    }
    return false;
}

void LocalScenesManager::sendSceneProgressCmd( const SceneType& scene, const char* status, bool isNotifyUsers ) {
    send<responces::SceneRunProgress>(
        "nma", scene.externStorId(), status, scene.name(), isNotifyUsers, scene.usersToNotify(), scene.roomId() );
}

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
void LocalScenesManager::updateConditions( const WhenBlock& condition ) {
    ParentType::updateConditions( condition );
    m_beaconsChecker.isTriggered( condition );
}

void LocalScenesManager::updateCheckers() {
    ParentType::updateCheckers();
    m_beaconsChecker.update();
}
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

void LocalScenesManager::onSceneChanged( const SceneType& newScene, const bool nameWasChanged ) {
    SMN_LOGI( "Scene id=%s was changed, nameWasChanged=%d", newScene.externStorId().c_str(), (int)nameWasChanged );

    send<responces::SceneChanged>( "nma", newScene.externStorId(), nameWasChanged );
}

void LocalScenesManager::onSceneChanged( const SceneType& newScene, const SceneType& oldScene ) {
    SMN_LOGI( "Scene id=%s was changed", newScene.externStorId().c_str() );

    const bool nameWasChanged = ( oldScene.name() != newScene.name() );
    onSceneChanged( newScene, nameWasChanged );
}

void LocalScenesManager::visit( const bus::BusAddr from, const bus::BusAddr to, commands::SetSceneName& command ) {
    SMN_LOGI( "visit( commands::SetSceneName )" );

    auto scenePtr = getSceneByStorId( command.sceneId() );
    if( !scenePtr ) {
        command.setResponce( errors::InvalidParams, "Scene doesn't exist" );
        return;
    }
    SceneType& scene = *scenePtr;

    scene.setName( command.sceneNewName() );
    if( !setScene( scenePtr ) ) {
        command.setResponce( errors::InternalError, "Can't save scene" );
        return;
    }

    constexpr bool nameWasChanged = true;
    onSceneChanged( scene, nameWasChanged );

    command.setResponce( errors::NotError, StringType() );
}

void LocalScenesManager::visit( const bus::BusAddr from,
                                const bus::BusAddr to,
                                commands::SetSceneHouseModes& command ) {
    SMN_LOGI( "visit( commands::SetSceneHouseModes )" );

    auto scenePtr = getSceneByStorId( command.sceneId() );
    if( !scenePtr ) {
        command.setResponce( errors::InvalidParams, "Scene doesn't exist" );
        return;
    }
    SceneType& scene = *scenePtr;
    auto sceneStorId = scene.storageId();

    scene.setHouseModes( command.houseModes() );
    if( !setScene( scenePtr ) ) {
        command.setResponce( errors::InternalError, "Can't save scene" );
        return;
    }

    constexpr bool nameWasChanged = false;
    onSceneChanged( scene, nameWasChanged );

    command.setResponce( errors::NotError, StringType() );
}

void LocalScenesManager::visit( const bus::BusAddr from, const bus::BusAddr to, commands::TimeList& command ) {
    SMN_LOGI( "visit( commands::TimeList )" );

    command.setResponce( jsonifyTimeList<SceneType>( m_scenesMap, m_conditionsMap, command.gen() ) );
}

void LocalScenesManager::visit( const bus::BusAddr from,
                                const bus::BusAddr to,
                                commands::EnumerateLocalScenes& command ) {
    SMN_LOGI( "visit( commands::EnumerateLocalScenes )" );

    for( const auto& itr : m_scenesMap ) {
        command.clbk()( *itr.second );
    }

    command.setResponce( errors::NotError, StringType() );
}

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
void LocalScenesManager::visit( const bus::BusAddr from,
                                const bus::BusAddr to,
                                const dev_dis::responce::DiscoveredDevices& responce ) {
    SMN_LOGI( "visit( dev_dis::responce::DiscoveredDevices& responce )" );
    m_beaconsChecker.onDevicesDiscovered( responce.registeredState(), responce.unknowState() );
}
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

void LocalScenesManager::visit( const bus::BusAddr from, const bus::BusAddr to, commands::ScenesData& command ) {
    SMN_LOGI( "visit( commands::ScenesData )" );
    // See docs: https://log.ezlo.com/new/hub/common/#hubdatalist

    auto& jsGen = command.gen();

    if( !jsGen.writeOpeningObject() ) {
        command.setResponce( std::make_tuple( errors::InternalError, StringType{} ) );
        return;
    }

    int errorCode{ errors::NotError };
    StringType errorDescr;

    bool isSuccess = true;
    for( const auto& [uuid, scene] : m_scenesMap ) {
        if( !command.isScene( scene->externStorId() ) ) {
            continue;
        }

        isSuccess = jsGen.writePropertyKey( scene->externStorId() ) && jsGen.writeOpeningObject();

        if( isSuccess && command.isEnabledField() ) {
            isSuccess = jsGen.writeProperty( tokens::Enabled, scene->isEnabled() );
        }

        if( isSuccess && command.isGroupIdField() ) {
            isSuccess = jsGen.writeProperty( tokens::GroupId, scene->groupId() );
        }

        if( isSuccess && command.isNameField() ) {
            isSuccess = jsGen.writeProperty( tokens::Name, scene->name() );
        }

        if( isSuccess && command.isParentIdField() ) {
            isSuccess = jsGen.writeProperty( tokens::ParentId, scene->roomId() );
        }

        isSuccess = isSuccess && jsGen.writeClosingSymbol();
        if( !isSuccess ) {
            break;
        }
    }

    if( !isSuccess || !jsGen.writeClosingSymbol() ) {
        command.setResponce( std::make_tuple( errors::InternalError, StringType{} ) );
        return;
    }

    command.setResponce( std::make_tuple( errorCode, std::move( errorDescr ) ) );
}

}  // namespace scenes
