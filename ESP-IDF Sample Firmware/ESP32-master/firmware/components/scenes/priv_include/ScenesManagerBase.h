#pragma once

#include "Common.h"
#include "Executor.h"
#include "ConditionChecker.h"
#include "Time.h"
#include "Scene.h"
#include "ScenesDBIterator.h"
#include "ScenesInfo.h"
#include "ActionsInfo.h"

#include "Shutdownable.h"
#include "BusCommand.h"

#include <map>
#include <mutex>

//#define ENABLE_SMN_LOGS
#ifdef ENABLE_SMN_LOGS

#include "DebugUtils.h"

#define SMN_LOGI( _FMT, ... ) ESP_LOGI( logTag, _FMT, ##__VA_ARGS__ )
#define SMN_LOGE( _FMT, ... ) ESP_LOGE( logTag, _FMT, ##__VA_ARGS__ )
#define SMN_LOGW( _FMT, ... ) ESP_LOGW( logTag, _FMT, ##__VA_ARGS__ )

#define SMN_PRINT_INFO( ... )                           \
    do {                                                \
        SMN_LOGI( __VA_ARGS__ );                        \
        m_stor->printInfo();                            \
        storage::Storage::get().printDir( scenesPath ); \
    } while( false )

#else  // ENABLE_SMN_LOGS

#define SMN_LOGI( ... ) \
    do {                \
    } while( false )
#define SMN_LOGE( ... ) \
    do {                \
    } while( false )
#define SMN_LOGW( ... ) \
    do {                \
    } while( false )

#define SMN_PRINT_INFO( ... ) \
    do {                      \
    } while( false )

#endif  // ENABLE_SMN_LOGS

namespace gason {
class JsonValue;
}  // namespace gason

namespace core {
class Ezlo;
}  // namespace core

namespace scenes {

namespace commands {
class SetSceneName;
class SetSceneHouseModes;
}  // namespace commands

template <typename SceneType, const char* scenesPath, const char* logTag>
class ScenesManagerBase : public core::Shutdownable {
public:
    using ParentType = core::Shutdownable;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    using ScenesInfoMap = std::map<Uuid, ScenesInfo>;
    using ActionsInfoMap = std::map<Uuid, ActionInfo>;
    using ActionsMap = std::map<Uuid, ThenBlock::UPtr>;

    enum class UpdateState {
        StageWaitingTime,
        StageInit,
        StageReset,
        StageUpdateConditions,
        StageUpdateLogicConditions,
        StageUpdateScenes,
        StageReboot
    };

    static constexpr const auto SaveBufferSize = 2 * BufferStreamType::MinBufSize;
#ifdef CONFIG_SPIRAM
    static constexpr const unsigned int MsgQueueSize = 256;
#else   // not CONFIG_SPIRAM
    static constexpr const unsigned int MsgQueueSize = 128;
#endif  // CONFIG_SPIRAM

    static constexpr const char* ProcessedScenesStorKey = "ActiveActions";
    static constexpr const char* ActiveActionsStorKey = "AActions";
    static constexpr const char* TriggeredConditionsStorKey = "TrigCond";

    ScenesManagerBase();

    ScenesManagerBase( ScenesManagerBase& ) = delete;

    ScenesManagerBase( ScenesManagerBase&& ) = delete;

    ScenesManagerBase& operator=( ScenesManagerBase& ) = delete;

    ScenesManagerBase& operator=( ScenesManagerBase&& ) = delete;

    ResultType createScene( gason::JsonValue& jsonRoot );

    ResultType removeScene( const StringType& sceneStorId );

    ResultType execScene( const StringType& storageId );

    typename SceneType::UPtr getSceneByStorId( const StringType& storId ) const;

    inline bool isSceneExist( const StringType& storId ) const {
        return nullptr != getSceneByStorId( storId );
    }

    void onTimeEstablished();

    UuidSet scenes() const;

protected:
    ClbkRet updateComponent( UpdContext& ctx ) override;

    void onShutdownStart() override;

protected:
    virtual bool setScene( typename SceneType::UPtr& scene );

    virtual bool removeScene( const SceneType& scene, storage::KeyValueStorage& stor );

    virtual bool load( storage::KeyValueStorage& stor );

    virtual bool reset();

    virtual void addScene( const SceneType& scene, storage::KeyValueStorage& stor, const bool isChanged );

    virtual ResultType editScene( gason::JsonValue& jsonRoot, const StringType& storSceneId );

    virtual ResultType enableScene( const StringType& sceneStorId, const bool isEnabled );

    virtual bool isSceneTriggered( const SceneType& scene ) const = 0;

    virtual void sendSceneProgressCmd( const SceneType& scene, const char* state, bool isNotifyUsers = true ) = 0;

protected:
    bool init( const char* ownBusName, StorUPtr&& stor );

    virtual bool connectToBus( const char* ownBusName );

    void removeProcessedScene( const Uuid& sceneUuid, storage::KeyValueStorage& stor );

    bool saveProcessedScenes( storage::KeyValueStorage& stor ) const;

    bool saveActiveActions( storage::KeyValueStorage& stor ) const;

    bool saveActiveConditions( storage::KeyValueStorage& stor ) const;

    ResultType execScene( const SceneType& scene, storage::KeyValueStorage& stor );

    void updateSceneActions( const SceneType& scene, ScenesInfo& sceneInfo, storage::KeyValueStorage& stor );

    void updateScenes1( const SceneType& scene, storage::KeyValueStorage& stor );

    void updateScenes2( const SceneType& scene, storage::KeyValueStorage& stor );

    void initConditions( const WhenBlock& condition );

    virtual void updateConditions( const WhenBlock& condition );

    void updateActions( const ThenBlock& action, storage::KeyValueStorage& stor );

    virtual void updateCheckers();

protected:
    virtual void onSceneCreated( const SceneType& scene );

    virtual void onSceneDeleted( const SceneType& scene );

    virtual void onSceneChanged( const SceneType& newScene, const bool nameWasChanged ) = 0;

    virtual void onSceneChanged( const SceneType& newScene, const SceneType& oldScene ) = 0;

    void onActionCompleted( const ThenBlock& block, const ActionInfo& info );

    void onActionPostponed( const ThenBlock& block, const ActionInfo& info );

    void onActionFailed( const ThenBlock& block, const ActionInfo& info );

    void onSceneStarted( const SceneType& scene, storage::KeyValueStorage& stor );

    void onSceneProgress( const SceneType& scene, const ScenesInfo& info );

    void onSceneCompleted( const SceneType& scene );

    void onSceneFailed( const SceneType& scene );

protected:  // bus::IBusClient
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateItem& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateDevice& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateItemDictionary& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::nma::CloudControllerAbstractSetResponce& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::CreateScene& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::EditScene& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::DeleteScene& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::ScenesList& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::SceneEnabledSet& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::RunScene& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, commands::JsonifyScene& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const time_service::responce::TimeEstablished& command ) override;

    void tryRecoverScenesFromOldSaves( storage::KeyValueStorage& stor );

    void cleanupDir( storage::KeyValueStorage& stor );

    void cleanUp( storage::KeyValueStorage& stor );

protected:
    bool m_isTimeEstablished = false;
    TimePointType m_prevTp, m_currTp;
    UuidSet m_prevTrig, m_currTrig;
    bool m_needtoSaveTriggers = false;

    UpdateState m_updateState = UpdateState::StageWaitingTime;

    TimeConditionChecker m_triggeredChecker;
    ItemsConditionChecker m_itemsChecker;
    LogicConditionChecker m_logicChecker;

    ScenesInfoMap m_processedScenes;
    ActionsInfoMap m_activeActions;

    Executor<ScenesManagerBase, ActionInfo> m_executor;

    storage::KeyValueStorageFactory::StoragePtr m_stor;

    ScenesMap<SceneType> m_scenesMap;
    ConditionsMap m_conditionsMap;
    ActionsMap m_actionsMap;
};

}  // end of namespace scenes

#include "../src/ScenesManagerBaseImpl.hpp"
