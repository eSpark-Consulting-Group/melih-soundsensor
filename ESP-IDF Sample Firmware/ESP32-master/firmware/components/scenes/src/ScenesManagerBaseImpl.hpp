#include "JsonParser.h"
#include "JsonGenerator.h"

#include "HouseModesManager.h"

#include "time_service/Utils.h"

#include "scenes/commands/SetSceneName.h"
#include "scenes/commands/SetSceneHouseModes.h"
#include "scenes/commands/CreateScene.h"

#include "scenes/commands/EditScene.h"
#include "scenes/commands/DeleteScene.h"
#include "scenes/commands/ScenesList.h"
#include "scenes/commands/SceneEnabledSet.h"
#include "scenes/commands/RunScene.h"
#include "scenes/commands/JsonifyScene.h"
#include "scenes/commands/TimeList.h"

#include "scenes/responces/SceneAdded.h"
#include "scenes/responces/SceneChanged.h"
#include "scenes/responces/SceneDeleted.h"
#include "scenes/responces/SceneRunProgress.h"

#include "lua/responce/UpdateItem.h"
#include "lua/responce/UpdateItemDictionary.h"
#include "BusCommandModesSwitched.h"
#include "lua/responce/UpdateDevice.h"
#include "BusCloudControllerAbstractSetResponce.h"

#include "CommonResources.h"
#include "IRamBufStream.h"
#include "JsonStreamGenerator.h"
#include "Storage.h"

#include "IValueCast.h"

#include <iterator>
#include <algorithm>

namespace scenes {

namespace {

using namespace common::resources;
using namespace std::chrono_literals;

}  // namespace

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::init( const char* ownBusName, StorUPtr&& stor ) {
    if( !stor ) {
        SMN_LOGE( "Invalid stor!" );
        return false;
    }
    m_stor = std::move( stor );

    if( !storage::Storage::get().createDir( scenesPath ) || !storage::Storage::get().createDir( SCENES_CACHE_PATH ) ) {
        SMN_LOGE( "Can't create scenes directories!" );
        return false;
    }

    if( !connectToBus( ownBusName ) ) {
        return false;
    }

    cleanUp( *m_stor );

    SMN_PRINT_INFO( "init(busName=%s, stor=?)", busName );

    tryRecoverScenesFromOldSaves( *m_stor );

    if( !load( *m_stor ) && !reset() ) {
        return false;
    }

    if( time_service::waitingTimeDidEstablishedFor( 0ms ) ) {
        onTimeEstablished();
    }

    return true;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::connectToBus( const char* ownBusName ) {
    if( !bus::Bus::get().join( ownBusName, *this ) ) {
        SMN_LOGE( "Wasn't connected to %s!", ownBusName );
        return false;
    }

    if( !bus::Bus::get().join( lua::responce::BUS_NAME, *this ) ) {
        SMN_LOGE( "Wasn't connected to \"%s\"!", lua::responce::BUS_NAME );
        return false;
    }

    if( !bus::Bus::get().join( time_service::responce::BUS_NAME, *this ) ) {
        SMN_LOGE( "Wasn't connected to \"%s\"!", time_service::responce::BUS_NAME );
        return false;
    }

    return true;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
ScenesManagerBase<SceneType, scenesPath, logTag>::ScenesManagerBase()
: core::Shutdownable( MsgQueueSize )
, m_triggeredChecker( m_prevTrig, m_currTrig )
, m_itemsChecker( m_prevTrig, m_currTrig )
, m_logicChecker( m_prevTrig, m_currTrig )
, m_executor( *this ) {}

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::load( storage::KeyValueStorage& stor ) {
    BlobType blob;
    if( stor.getBlob( ProcessedScenesStorKey, blob ) ) {
        common::ArrayRStream stream( blob.data(), blob.size() );
        if( stream.read( m_processedScenes ) ) {
            SMN_LOGI( "Load %d processed scenes from storage successfully.", m_processedScenes.size() );
        }
        else {
            SMN_LOGE( "Can't load processed scenes from storage!" );
            return false;
        }
    }
    else {
        SMN_LOGW( "Can't get processed scenes blob from storage!" );
    }

    if( stor.getBlob( ActiveActionsStorKey, blob ) ) {
        common::ArrayRStream stream( blob.data(), blob.size() );
        if( stream.read( m_activeActions ) ) {
            SMN_LOGI( "Load %d active actions from storage successfully.", m_activeActions.size() );
        }
        else {
            SMN_LOGE( "Can't load active actions from storage!" );
            return false;
        }
    }
    else {
        SMN_LOGW( "Can't get active actions blob from storage!" );
    }

    if( stor.getBlob( TriggeredConditionsStorKey, blob ) ) {
        common::ArrayRStream stream( blob.data(), blob.size() );
        if( stream.read( m_prevTrig ) ) {
            SMN_LOGI( "Load active conditions %d from storage successfully.", m_prevTrig.size() );
        }
        else {
            SMN_LOGE( "Can't load active conditions from storage!" );
            return false;
        }
    }
    else {
        SMN_LOGW( "Can't get active conditions blob from storage!" );
    }

    ScenesDBClbkIterator dbItr( *m_stor );
    dbItr.setSceneClbk(
        [this]( const StringType& storKey, const ScenesDBClbkIterator::ItrType& sceneItr, StorType& stor ) {
            auto sceneVal = sceneItr.value();
            auto scene = SceneType::create( sceneVal );
            if( scene ) {
                m_scenesMap.emplace( storKey, std::move( scene ) );
            }
        } );
    dbItr.setConditionClbk(
        [this]( const StringType& storKey, const ScenesDBClbkIterator::ItrType& condItr, StorType& stor ) {
            auto condVal = condItr.value();
            auto cond = WhenBlock::create( condVal );
            if( cond ) {
                m_conditionsMap.emplace( storKey, std::move( cond ) );
            }
        } );
    dbItr.setActionClbk(
        [this]( const StringType& storKey, const ScenesDBClbkIterator::ItrType& actItr, StorType& stor ) {
            auto actVal = actItr.value();
            auto act = ThenBlock::create( actVal );
            if( act ) {
                m_actionsMap.emplace( storKey, std::move( act ) );
            }
        } );
    dbItr.reset();
    while( dbItr.next() ) {
    }

    return true;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::removeProcessedScene( const Uuid& sceneUuid,
                                                                             storage::KeyValueStorage& stor ) {
    SMN_LOGI( "removeProcessedScene(sceneUuid=%s, stor=?)", sceneUuid.toString( SceneType::StorPrefix ).c_str() );

    auto erasedCount = m_processedScenes.erase( sceneUuid );
    if( 0 != erasedCount ) {
        saveProcessedScenes( stor );
    }
}

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::saveProcessedScenes( storage::KeyValueStorage& stor ) const {
    BufferStreamType stream( SaveBufferSize );
    bool is_successfull =
        stream && stream.write( m_processedScenes ) && stor.setBlob( ProcessedScenesStorKey, stream, stream.size() );
    if( is_successfull ) {
        SMN_LOGI( "saveProcessedScenes(): Save processed scenes to storage." );
    }
    else {
        SMN_LOGE( "saveProcessedScenes(): Doesn't save processed scenes to storage!" );
    }
    return is_successfull;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::saveActiveActions( storage::KeyValueStorage& stor ) const {
    BufferStreamType stream( SaveBufferSize );
    bool is_successfull =
        stream && stream.write( m_activeActions ) && stor.setBlob( ActiveActionsStorKey, stream, stream.size() );
    if( is_successfull ) {
        SMN_LOGI( "saveActiveActions(): Save active actions to storage." );
    }
    else {
        SMN_LOGE( "saveActiveActions(): Doesn't save active actions to storage!" );
    }
    return is_successfull;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::saveActiveConditions( storage::KeyValueStorage& stor ) const {
    BufferStreamType stream( SaveBufferSize );
    bool is_successfull =
        stream && stream.write( m_prevTrig ) && stor.setBlob( TriggeredConditionsStorKey, stream, stream.size() );
    if( is_successfull ) {
        SMN_LOGI( "saveActiveConditions(): Save active conditions to storage." );
    }
    else {
        SMN_LOGE( "saveActiveConditions(): Doesn't save active conditions to storage!" );
    }
    return is_successfull;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::addScene( const SceneType& scene,
                                                                 storage::KeyValueStorage& stor,
                                                                 const bool isChanged ) {
    SMN_LOGI( "addScene(scene=%s, stor=?, isChanged=%d)", scene.storageId().c_str(), int( isChanged ) );

    ScenesDBClbkIterator dbItr( *m_stor );
    dbItr.setConditionClbk(
        [this]( const StringType& storKey, const ScenesDBClbkIterator::ItrType& condItr, StorType& stor ) {
            Uuid condUuid( storKey );
            if( condUuid && m_conditionsMap.end() == m_conditionsMap.find( condUuid ) ) {
                auto condVal = condItr.value();
                auto cond = WhenBlock::create( condVal );
                if( cond ) {
                    m_conditionsMap.emplace( storKey, std::move( cond ) );
                }
            }
        } );
    dbItr.reset();
    while( dbItr.next() ) {
    }

    for( const auto& actId : scene.actions() ) {
        auto thenBlock = ThenBlock::create( actId, stor );
        if( !thenBlock ) {
            continue;
        }
        m_actionsMap.emplace( thenBlock->uuid(), std::move( thenBlock ) );
    }

    m_scenesMap.emplace( scene.uuid(), scene.clone() );

    {
        RecursiveConditionInitor initor( stor, m_itemsChecker );
        for( const auto& storKey : scene.conditions() ) {
            auto whenBlock = WhenBlock::create( storKey, stor );
            if( whenBlock ) {
                initor.init( *whenBlock );
            }
        }
    }

    auto state = isChanged ? ScenesInfo::State::RecentlyEdited : ScenesInfo::State::RecentlyAdded;
    m_processedScenes.emplace( scene.uuid(), ScenesInfo( state ) );
    saveProcessedScenes( stor );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
ResultType ScenesManagerBase<SceneType, scenesPath, logTag>::editScene( gason::JsonValue& jsonRoot,
                                                                        const StringType& storSceneId ) {
    SMN_PRINT_INFO( "editScene(?) ------- stor before" );

    auto oldScene = getSceneByStorId( storSceneId );
    if( !oldScene ) {
        return std::make_tuple( errors::ApplicationError, "Scene doesn't exist!" );
    }
    auto newSceneUuid = Uuid::generate();

    {
        int errorCode;
        std::string errorDescr;
        std::tie( errorCode, errorDescr ) =
            saveSceneToDbAndFile( scenesPath, jsonRoot, newSceneUuid, SceneType::storKey );
        if( errors::NotError != errorCode ) {
            return std::make_tuple( errorCode, errorDescr );
        }
    }

    typename SceneType::UPtr newScene;
    {
        int errorCode;
        std::string errorDescr;
        std::tie( errorCode, errorDescr, newScene ) = parseScene<SceneType>( jsonRoot, newSceneUuid, *m_stor );
        if( errors::NotError != errorCode || !newScene ) {
            return std::make_tuple( errorCode, errorDescr );
        }
    }

    if( newScene->persistentStorId().empty() ) {
        newScene->setPersistentStorId( oldScene->externStorId() );
    }
    oldScene->setPersistentStorId( std::string() );

    if( !newScene->save( *m_stor ) ) {
        return std::make_tuple( errors::ApplicationError, "Can't save edited scene!" );
    }

    constexpr bool isChanged = true;
    addScene( *newScene, *m_stor, isChanged );

    if( !removeScene( *oldScene, *m_stor ) ) {
        SMN_LOGI( "Can't remove old edited scene (storageId=%s)", oldScene->storageId().c_str() );
    }

    SMN_PRINT_INFO( "editScene(?) ------- stor after" );

    onSceneChanged( *newScene, *oldScene );

    return std::make_tuple( errors::NotError, "" );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
ResultType ScenesManagerBase<SceneType, scenesPath, logTag>::enableScene( const StringType& sceneStorId,
                                                                          const bool isEnabled ) {
    SMN_LOGI( "enableScene(sceneStorId=%s, isEnabled=%d)", sceneStorId.c_str(), int( isEnabled ) );

    auto scenePtr = getSceneByStorId( sceneStorId );
    if( !scenePtr ) {
        return std::make_tuple( errors::ApplicationError, "Scene doesn't exist!" );
    }
    SceneType& scene = *scenePtr;

    if( isEnabled == scene.isEnabled() ) {
        return std::make_tuple( errors::NotError, StringType() );
    }

    scene.setEnabled( isEnabled );
    if( !scene.save( *m_stor ) ) {
        return std::make_tuple( errors::InternalError, "Can't save scene to database!" );
    }
    m_scenesMap[scene.uuid()] = std::move( scenePtr );

    constexpr const bool isNameChanged = false;
    onSceneChanged( scene, isNameChanged );

    return std::make_tuple( errors::NotError, StringType() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
ResultType ScenesManagerBase<SceneType, scenesPath, logTag>::execScene( const StringType& storageId ) {
    SMN_LOGI( "execScene(storageId=%s)", storageId.c_str() );

    auto scene = getSceneByStorId( storageId );
    if( !scene ) {
        return std::make_tuple( errors::InvalidParams, "Scene doesn't exist!" );
    }
    return execScene( *scene, *m_stor );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
typename SceneType::UPtr ScenesManagerBase<SceneType, scenesPath, logTag>::getSceneByStorId(
    const StringType& storId ) const {
    {
        Uuid sceneUuid( storId );
        if( sceneUuid ) {
            auto itr = m_scenesMap.find( sceneUuid );
            if( m_scenesMap.end() != itr ) {
                return itr->second ? itr->second->clone() : nullptr;
            }
        }
    }

    {
        BlobType blob;
        if( !m_stor->getBlob( SceneType::PersistentStorPrefix + storId, blob ) ) {
            return nullptr;
        }
        common::ArrayRStream stream( blob.data(), blob.size() );
        StringType mappedStorId;
        if( !stream.read( mappedStorId ) ) {
            return nullptr;
        }
        Uuid sceneUuid( mappedStorId );
        if( sceneUuid ) {
            auto itr = m_scenesMap.find( sceneUuid );
            if( m_scenesMap.end() != itr ) {
                return itr->second ? itr->second->clone() : nullptr;
            }
        }
    }

    return nullptr;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
ResultType ScenesManagerBase<SceneType, scenesPath, logTag>::execScene( const SceneType& scene,
                                                                        storage::KeyValueStorage& stor ) {
    SMN_LOGI( "runScene: uuid=%s", scene.storageId().c_str() );

    bool wasInserted = false;
    typename decltype( m_processedScenes )::iterator infoItr;
    std::tie( infoItr, wasInserted ) = m_processedScenes.emplace( scene.uuid(), ScenesInfo() );
    if( !wasInserted ) {
        return std::make_tuple( errors::ApplicationError, "Scene already executed!" );
    }

    saveProcessedScenes( stor );
    onSceneStarted( scene, stor );
    if( scene.actions().empty() ) {
        infoItr->second.setCompleted();
        onSceneCompleted( scene );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::updateSceneActions( const SceneType& scene,
                                                                           ScenesInfo& sceneInfo,
                                                                           storage::KeyValueStorage& stor ) {
    const auto& actions = scene.actions();
    auto itr = actions.begin();
    std::advance( itr, sceneInfo.actionIndex() );
    if( actions.end() == itr ) {
        sceneInfo.setFailed();
        onSceneFailed( scene );
        return;
    }
    auto& actionInfo = m_activeActions[*itr];

    if( actionInfo.isFailed() ) {
        if( 0 < m_activeActions.erase( *itr ) ) {
            saveActiveActions( stor );
        }
        sceneInfo.setFailed();
        onSceneFailed( scene );
        return;
    }

    if( !actionInfo.isCompleted() ) {
        return;
    }

    if( 0 < m_activeActions.erase( *itr ) ) {
        saveActiveActions( stor );
    }

    sceneInfo.setActionIndex( sceneInfo.actionIndex() + 1 );
    onSceneProgress( scene, sceneInfo );

    std::advance( itr, 1 );
    if( actions.end() == itr ) {
        sceneInfo.setCompleted();
        onSceneCompleted( scene );
        return;
    }

    bool isIgnoreResult = SceneType::ExecPolicy::IgnoreActionResult == scene.execPolicy();
    m_activeActions.emplace( *itr, ActionInfo( m_currTp, isIgnoreResult ) );
    saveActiveActions( stor );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::updateScenes1( const SceneType& scene,
                                                                      storage::KeyValueStorage& stor ) {
    auto infoItr = m_processedScenes.find( scene.uuid() );
    if( m_processedScenes.end() != infoItr ) {
        if( infoItr->second.isActive() ) {
            updateSceneActions( scene, infoItr->second, stor );
        }
    }
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::updateScenes2( const SceneType& scene,
                                                                      storage::KeyValueStorage& stor ) {
    auto infoItr = m_processedScenes.find( scene.uuid() );
    if( m_processedScenes.end() == infoItr ) {
        if( isSceneTriggered( scene ) ) {
            ScenesManagerBase<SceneType, scenesPath, logTag>::execScene( scene, stor );
        }
        return;
    }
    auto& sceneInfo = infoItr->second;

    if( sceneInfo.isActive() ) {
        updateSceneActions( scene, sceneInfo, stor );
    }
    else if( sceneInfo.state() == ScenesInfo::State::RecentlyAdded ) {
        sceneInfo.setState( ScenesInfo::State::AdditionInitialized );
        saveProcessedScenes( stor );
    }
    else if( sceneInfo.state() == ScenesInfo::State::RecentlyEdited ) {
        sceneInfo.setState( ScenesInfo::State::EditionInitialized );
        saveProcessedScenes( stor );
    }
    else {
        auto sceneUuid = scene.uuid();
        // Fall there if state is Completed, Failed or
        removeProcessedScene( sceneUuid, stor );
    }
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::initConditions( const WhenBlock& condition ) {
    ConditionInitor initor( m_itemsChecker );
    initor.init( condition );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::updateConditions( const WhenBlock& condition ) {
    m_triggeredChecker.isTriggered( condition );
    m_itemsChecker.isTriggered( condition );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::updateActions( const ThenBlock& action,
                                                                      storage::KeyValueStorage& stor ) {
    auto actionUuid = action.uuid();
    if( m_activeActions.end() == m_activeActions.find( actionUuid ) ) {
        return;
    }

    auto& info = m_activeActions[actionUuid];
    if( info.isFailed() || info.isCompleted() ) {
        return;
    }

    m_executor.execute( action, &info, m_prevTp, m_currTp );

    switch( info.state() ) {
    case ActionInfo::State::Postponed:
        onActionPostponed( action, info );
        break;
    case ActionInfo::State::Completed:
        onActionCompleted( action, info );
        break;
    case ActionInfo::State::Failed:
        onActionFailed( action, info );
        break;
    default:
        break;
    }
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::updateCheckers() {
    m_triggeredChecker.update( m_currTp );
    m_itemsChecker.update();
    m_logicChecker.update();
}

template <typename SceneType, const char* scenesPath, const char* logTag>
ResultType ScenesManagerBase<SceneType, scenesPath, logTag>::createScene( gason::JsonValue& jsonRoot ) {
    {
        const auto persistentSceneId = jsonRoot( tokens::SubId ).toString();
        if( persistentSceneId && isSceneExist( persistentSceneId ) ) {
            std::ostringstream ss;
            ss << "Scene with \"_id\"=" << persistentSceneId << " already exist!";
            return std::make_tuple( errors::InvalidParams, ss.str() );
        }
    }

    SMN_PRINT_INFO( "createScene(?) ------- stor before" );

    int errorCode;
    StringType errorDescr;
    typename SceneType::UPtr scene;

    auto sceneUuid = Uuid::generate();

    std::tie( errorCode, errorDescr ) = saveSceneToDbAndFile( scenesPath, jsonRoot, sceneUuid, SceneType::storKey );
    if( errors::NotError != errorCode ) {
        return std::make_tuple( errorCode, errorDescr );
    }

    std::tie( errorCode, errorDescr, scene ) = parseScene<SceneType>( jsonRoot, sceneUuid, *m_stor );
    if( errors::NotError != errorCode ) {
        return std::make_tuple( errorCode, errorDescr );
    }

    if( !scene->save( *m_stor ) ) {
        return std::make_tuple( errors::ApplicationError, "Can't save scene!" );
    }

    constexpr bool isChanged = false;
    addScene( *scene, *m_stor, isChanged );

    SMN_PRINT_INFO( "createScene(?) ------- stor after" );

    onSceneCreated( *scene );
    return std::make_tuple( errors::NotError, scene->externStorId() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
ResultType ScenesManagerBase<SceneType, scenesPath, logTag>::removeScene( const StringType& sceneStorId ) {
    SMN_PRINT_INFO( "removeScene(?) ------- stor before" );

    auto scene = getSceneByStorId( sceneStorId );
    if( !scene || !removeScene( *scene, *m_stor ) ) {
        return std::make_tuple( errors::ApplicationError, "Can't delete scene!" );
    }

    SMN_PRINT_INFO( "removeScene(?) ------- stor after" );

    onSceneDeleted( *scene );
    return std::make_tuple( errors::NotError, scene->externStorId() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::setScene( typename SceneType::UPtr& scene ) {
    if( scene->save( *m_stor ) ) {
        m_scenesMap[scene->uuid()] = std::move( scene );
        return true;
    }
    return false;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::removeScene( const SceneType& scene,
                                                                    storage::KeyValueStorage& stor ) {
    SMN_LOGI( "LocalScenesManager::removeScene: sceneStorageId=%s", scene.storageId().c_str() );

    for( const auto& condUuid : scene.conditions() ) {
        m_conditionsMap.erase( condUuid );
    }
    for( const auto& actUuid : scene.actions() ) {
        m_actionsMap.erase( actUuid );
    }
    m_scenesMap.erase( scene.uuid() );

    removeProcessedScene( scene.uuid(), stor );
    return scene.remove( scenesPath, stor );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onTimeEstablished() {
    SMN_LOGI( "onTimeEstablished()" );

    auto currTp = currentTime();
    m_prevTp = m_currTp = currTp;
    m_triggeredChecker.setCurrentTime( currTp );
    m_isTimeEstablished = true;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
UuidSet ScenesManagerBase<SceneType, scenesPath, logTag>::scenes() const {
    UuidSet scenesList;
    const auto end = m_scenesMap.cend();
    for( auto itr = m_scenesMap.cbegin(); end != itr; ++itr ) {
        scenesList.emplace( itr->first );
    }

    return scenesList;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
typename ScenesManagerBase<SceneType, scenesPath, logTag>::ClbkRet
ScenesManagerBase<SceneType, scenesPath, logTag>::updateComponent( UpdContext& ctx ) {
    switch( m_updateState ) {
    case UpdateState::StageWaitingTime: {
        if( m_isTimeEstablished ) {
            m_updateState = UpdateState::StageInit;
        }
        break;
    }
    case UpdateState::StageInit: {
        for( const auto& itr : m_conditionsMap ) {
            const WhenBlock& condition = *itr.second;
            initConditions( condition );
        }
        m_updateState = UpdateState::StageUpdateConditions;
        break;
    }
    case UpdateState::StageReset: {
        m_prevTp = m_currTp;
        m_currTp = currentTime();

        bool isNotEqual = ( m_prevTrig != m_currTrig );
        m_prevTrig.swap( m_currTrig );
        m_currTrig.clear();
        if( isNotEqual ) {
            saveActiveConditions( *m_stor );
        }

        updateCheckers();

        for( const auto& itr : m_actionsMap ) {
            const ThenBlock& action = *( itr.second );
            updateActions( action, *m_stor );
        }
        m_executor.send( *this );

        m_updateState = UpdateState::StageUpdateConditions;
        break;
    }
    case UpdateState::StageUpdateConditions: {
        for( const auto& itr : m_conditionsMap ) {
            const WhenBlock& condition = *( itr.second );
            updateConditions( condition );
        }
        m_updateState = UpdateState::StageUpdateLogicConditions;
        break;
    }
    case UpdateState::StageUpdateLogicConditions: {
        while( !m_logicChecker.isCompleted() ) {
            for( const auto& itr : m_conditionsMap ) {
                const WhenBlock& condition = *( itr.second );
                m_logicChecker.isTriggered( condition );
            }
            m_logicChecker.onPassCompleted();
        }
        m_updateState = UpdateState::StageUpdateScenes;
        break;
    }
    case UpdateState::StageUpdateScenes: {
        for( const auto& itr : m_scenesMap ) {
            const SceneType& scene = *( itr.second );
            updateScenes1( scene, *m_stor );
            updateScenes2( scene, *m_stor );
        }
        m_updateState = UpdateState::StageReset;
        break;
    }
    default:
        break;
    }

    constexpr auto waitTicks = 0 / portTICK_PERIOD_MS;
    tryReceive( waitTicks );

    return ClbkRet::Continue;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onShutdownStart() {
    SMN_LOGI( "onShutdownStart()" );

    core::Shutdownable::onShutdownStart();

    m_updateState = UpdateState::StageReboot;
}

template <typename SceneType, const char* scenesPath, const char* logTag>
bool ScenesManagerBase<SceneType, scenesPath, logTag>::reset() {
    SMN_LOGW( "reset" );

    m_scenesMap.clear();
    m_conditionsMap.clear();
    m_actionsMap.clear();
    m_processedScenes.clear();

    return m_stor->removeAll();
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onSceneCreated( const SceneType& scene ) {
    SMN_LOGI( "Scene id=%s was created", scene.externStorId().c_str() );

    send<responces::SceneAdded>( "nma", scene.externStorId() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onSceneDeleted( const SceneType& scene ) {
    SMN_LOGI( "Scene id=%s was deleted", scene.externStorId().c_str() );

    send<responces::SceneDeleted>( "nma", scene.externStorId() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onActionCompleted( const ThenBlock& block,
                                                                          const ActionInfo& info ) {
    SMN_LOGI( "Action %s was completed", block.storKey().c_str() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onActionPostponed( const ThenBlock& block,
                                                                          const ActionInfo& info ) {
    SMN_LOGI( "Action %s was postponed", block.storKey().c_str() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onActionFailed( const ThenBlock& block,
                                                                       const ActionInfo& info ) {
    SMN_LOGI( "Action %s was failed", block.storKey().c_str() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onSceneStarted( const SceneType& scene,
                                                                       storage::KeyValueStorage& stor ) {
    SMN_LOGI( "Scene id=%s started", scene.storageId().c_str() );
    sendSceneProgressCmd( scene, broadcasts::events::Started, false );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onSceneProgress( const SceneType& scene,
                                                                        const ScenesInfo& info ) {
    SMN_LOGI( "Scene id=%s, progress = %i",
              scene.storageId().c_str(),
              ( 100 * info.actionIndex() / scene.actions().size() ) );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onSceneCompleted( const SceneType& scene ) {
    SMN_LOGI( "Scene id=%s was completed", scene.storageId().c_str() );
    sendSceneProgressCmd( scene, broadcasts::events::Finished );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::onSceneFailed( const SceneType& scene ) {
    SMN_LOGI( "Scene id=%s was failed", scene.storageId().c_str() );
    sendSceneProgressCmd( scene, broadcasts::events::Failed );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              lua::responce::UpdateItem& command ) {
    SMN_LOGI( "visit( lua::responce::UpdateItem )" );
    auto value = command.value();
    if( value ) {
        m_itemsChecker.onItemUpdated( command.m_itemInfo->getId(), std::move( value ) );
    }
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              lua::responce::UpdateDevice& command ) {
    SMN_LOGI( "visit( lua::responce::UpdateDevice )" );
    if( command.m_value ) {
        m_itemsChecker.onDeviceUpdated( command.m_deviceId, command.m_key, command.m_value->clone() );
    }
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              lua::responce::UpdateItemDictionary& command ) {
    SMN_LOGI( "visit( lua::responce::UpdateItemDictionary )" );
    if( command.m_value ) {
        m_itemsChecker.onItemUpdated( command.m_itemInfo->getId(), command.m_value->clone() );
    }
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit(
    const bus::BusAddr from,
    const bus::BusAddr to,
    bus::command::nma::CloudControllerAbstractSetResponce& command ) {
    SMN_LOGI( "Scenes manager Handle CloudControllerAbstractSetResponce id=%s  isSuccess=%d",
              command.id().c_str(),
              (int)command.isSuccess() );
    m_executor.onCompleted( command.id(), command.isSuccess() );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              commands::CreateScene& command ) {
    SMN_LOGI( "visit( commands::CreateScene )" );
    command.setResponce( createScene( command.params() ) );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              commands::EditScene& command ) {
    SMN_LOGI( "visit( commands::EditScene )" );

    command.setResponce( editScene( command.params(), command.sceneId() ) );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              commands::DeleteScene& command ) {
    SMN_LOGI( "visit( commands::DeleteScene )" );

    command.setResponce( removeScene( command.sceneId() ) );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              commands::ScenesList& command ) {
    SMN_LOGI( "visit( commands::ScenesList )" );

    command.setResponce( jsonifyAllScenes<SceneType>( scenesPath, *m_stor, command.gen(), command.isOnlyId() ) );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              commands::SceneEnabledSet& command ) {
    SMN_LOGI( "visit( commands::SceneEnabledSet )" );

    command.setResponce( enableScene( command.sceneId(), command.isEnabled() ) );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              commands::RunScene& command ) {
    SMN_LOGI( "visit( commands::RunScene )" );

    command.setResponce( execScene( command.sceneId() ) );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              commands::JsonifyScene& command ) {
    SMN_LOGI( "visit( commands::JsonifyScene ) sceneId=%s", command.sceneId().c_str() );

    constexpr auto isOnlyId = false;
    command.setResponce( jsonifyScene<SceneType>( scenesPath, *m_stor, command.sceneId(), command.gen(), isOnlyId ) );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::visit( const bus::BusAddr from,
                                                              const bus::BusAddr to,
                                                              const time_service::responce::TimeEstablished& command ) {
    SMN_LOGI( "visit( time_service::responce::TimeEstablished )" );
    onTimeEstablished();
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::tryRecoverScenesFromOldSaves( storage::KeyValueStorage& stor ) {
    SMN_LOGI( "tryRecoverScenesFromOldSaves" );

    ScenesDBClbkIterator dbItr( stor );
    dbItr.setActionClbk(
        [this]( const StringType& storKey, const ScenesDBClbkIterator::ItrType& actItr, StorType& stor ) {
            auto actVal = actItr.value();
            auto oldActionPtr = ThenBlock::create( actVal );
            if( !oldActionPtr ) {
                return;
            }
            if( ThenBlock::Type::SetValueMulticast == oldActionPtr->type() ) {
                ThenBlockSetValueMulticast& oldAction = static_cast<ThenBlockSetValueMulticast&>( *oldActionPtr );
                ThenBlockSetValueMulticast3 newAction( oldAction.uuid(), oldAction.sceneUuid(), oldAction.delay() );
                auto& newValToItems = newAction.valueToItems();
                auto& oldValToItems = oldAction.valueToItems();
                for( auto& [value, oldItems] : oldValToItems ) {
                    {
                        ThenBlockSetValueMulticast3::KeyType key( value->getId(), value->clone() );
                        auto& newItems = newValToItems[std::move( key )];
                        newItems.insert( newItems.end(), oldItems.begin(), oldItems.end() );
                    }
                    {
                        ThenBlockSetValueMulticast3::KeyType key( value->getId(), value->clone() );
                        newAction.addToIndex( std::move( key ) );
                    }
                }
                if( !newAction.save( stor ) ) {
                    SMN_LOGE( "Can't save recovered ThenBlockSetValueMulticast3 action" );
                }
            }
            else if( ThenBlock::Type::SetValueMulticast2 == oldActionPtr->type() ) {
                ThenBlockSetValueMulticast2& oldAction = static_cast<ThenBlockSetValueMulticast2&>( *oldActionPtr );
                ThenBlockSetValueMulticast3 newAction( oldAction.uuid(), oldAction.sceneUuid(), oldAction.delay() );
                auto& newValToItems = newAction.valueToItems();
                auto& oldValToItems = oldAction.valueToItems();
                for( auto& [value, oldItems] : oldValToItems ) {
                    {
                        ThenBlockSetValueMulticast3::KeyType key( value.first, value.second->clone() );
                        auto& newItems = newValToItems[std::move( key )];
                        newItems.insert( newItems.end(), oldItems.begin(), oldItems.end() );
                    }
                    {
                        ThenBlockSetValueMulticast3::KeyType key( value.first, value.second->clone() );
                        newAction.addToIndex( std::move( key ) );
                    }
                }
                if( !newAction.save( stor ) ) {
                    SMN_LOGE( "Can't save recovered ThenBlockSetValueMulticast3 action" );
                }
            }
        } );

    dbItr.reset();
    while( dbItr.next() ) {
    }
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::cleanupDir( storage::KeyValueStorage& stor ) {
    SMN_LOGI( "Check scenes directory: %s", scenesPath );
    auto onFileClbk = [&stor, this]( const char* name, const char* fullPath ) {
        SMN_LOGI( "check scene file %s", name );
        if( !stor.exists( name ) ) {
            SMN_LOGW( "remove scene file %s", fullPath );
            storage::Storage::get().remove( fullPath );
        }

        return storage::Storage::WalkDir::Continue;
    };
    storage::Storage::get().walkDir( scenesPath, onFileClbk, nullptr );
}

template <typename SceneType, const char* scenesPath, const char* logTag>
void ScenesManagerBase<SceneType, scenesPath, logTag>::cleanUp( storage::KeyValueStorage& stor ) {
    SMN_LOGI( ">>>>> Cleanup scenes start" );

    cleanupDir( stor );

    UuidSet items;

    SMN_LOGI( "check scenes" );

    ScenesDBClbkIterator DBIterator( stor );
    DBIterator.setActionClbk( nullptr );
    DBIterator.setConditionClbk( nullptr );
    DBIterator.setSceneClbk( [&items, this]( const StringType& storKey,
                                             const ScenesDBClbkIterator::ItrType& itr,
                                             storage::KeyValueStorage& stor ) {
        {
            std::string sceneJsFile( scenesPath );
            sceneJsFile += "/";
            sceneJsFile += storKey;
            if( !storage::Storage::get().isExist( sceneJsFile ) ) {
                stor.remove( storKey );
                SMN_LOGW( "remove scene: %s", storKey.c_str() );
                return;
            }
        }

        typename SceneType::UPtr scene;
        {
            auto blob = itr.value();
            scene = SceneType::create( blob );
            if( !scene ) {
                stor.remove( storKey );
                SMN_LOGW( "remove scene: %s", storKey.c_str() );
                return;
            }
        }

        for( const auto& condUuid : scene->conditions() ) {
            auto condStorKey = WhenBlock::storKey( condUuid );
            if( !stor.exists( condStorKey ) ) {
                scene->remove( scenesPath, stor );
                SMN_LOGW( "remove scene: %s", storKey.c_str() );
                return;
            }
        }

        for( const auto& actUuid : scene->actions() ) {
            auto actStorKey = ThenBlock::storKey( actUuid );
            if( !stor.exists( actStorKey ) ) {
                scene->remove( scenesPath, stor );
                SMN_LOGW( "remove scene: %s", storKey.c_str() );
                return;
            }
        }

        SMN_LOGI( "scene %s is valid", storKey.c_str() );
        items.emplace( scene->uuid() );
    } );
    DBIterator.reset();
    while( DBIterator.next() ) {
    }
    DBIterator.setSceneClbk( nullptr );

    SMN_LOGI( "check actions" );
    DBIterator.setActionClbk( [&items, this]( const StringType& storKey,
                                              const ScenesDBClbkIterator::ItrType& itr,
                                              storage::KeyValueStorage& stor ) {
        ThenBlock::UPtr thenBlock;
        {
            auto blob = itr.value();
            thenBlock = ThenBlock::create( blob );
            if( !thenBlock ) {
                stor.remove( storKey );
                SMN_LOGW( "remove action: %s", storKey.c_str() );
                return;
            }
        }

        if( items.end() == items.find( thenBlock->sceneUuid() ) ) {
            stor.remove( storKey );
            SMN_LOGW( "remove action: %s", storKey.c_str() );
            return;
        }

        SMN_LOGI( "action %s is valid", storKey.c_str() );
    } );

    DBIterator.reset();
    while( DBIterator.next() ) {
    }
    DBIterator.setActionClbk( nullptr );

    SMN_LOGI( "check conditions" );
    DBIterator.setConditionClbk( [&items, this]( const StringType& storKey,
                                                 const ScenesDBClbkIterator::ItrType& itr,
                                                 storage::KeyValueStorage& stor ) {
        WhenBlock::UPtr whenBlock;
        {
            auto blob = itr.value();
            whenBlock = WhenBlock::create( blob );
            if( !whenBlock ) {
                stor.remove( storKey );
                SMN_LOGW( "remove condition: %s", storKey.c_str() );
                return;
            }
        }

        items.emplace( whenBlock->uuid() );
    } );
    DBIterator.reset();
    while( DBIterator.next() ) {
    }

    DBIterator.setConditionClbk( [&items, this]( const StringType& storKey,
                                                 const ScenesDBClbkIterator::ItrType& itr,
                                                 storage::KeyValueStorage& stor ) {
        WhenBlock::UPtr whenBlock;
        {
            auto blob = itr.value();
            whenBlock = WhenBlock::create( blob );
            if( !whenBlock ) {
                stor.remove( storKey );
                SMN_LOGW( "remove condition: %s", storKey.c_str() );
                return;
            }
        }

        if( items.end() == items.find( whenBlock->parentUuid() ) ) {
            stor.remove( storKey );
            SMN_LOGW( "remove condition: %s", storKey.c_str() );
            return;
        }

        SMN_LOGI( "condition %s is valid", storKey.c_str() );
    } );
    DBIterator.reset();
    while( DBIterator.next() ) {
    }

    cleanupDir( stor );

    SMN_LOGI( "<<<<< Cleanup scenes complete" );
}

}  // end of namespace scenes
