#include "Executor.h"
#include "HouseModesManager.h"
#include "Cache.h"
#include "BinaryFileStorageFactory.h"
#include "DeviceInfo.h"
#include "StorageSection.h"
#include "ObjectSerializer.h"
#include "IValueCast.h"
#include "values/BoolValue.h"
#include "CommonResources.h"
#include "JsonStreamGenerator.h"
#include "HTTPRequest.h"

#include "BusCommandNmaClientClbk.h"
#include "BusCommandCloudItemValueSet.h"

#include <memory>

#include <esp_system.h>

#include "DebugUtils.h"

namespace scenes {

using namespace common::resources;

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockSetItemValue& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    if( !m_actionsInfo->isIgnoreResult() ) {
        if( !isDeviceOfItemReachable( block.itemId() ) ) {
            if( m_actionsInfo->isPostponed() ) {
                m_actionsInfo->setFailed();
                FUNC_LOGW( "Action for itemId=%s failed because device isn't ready!", block.itemId().c_str() );
                return;
            }
            else {
                FUNC_LOGI( "Action for item_id=%s postponed", block.itemId().c_str() );
                m_actionsInfo->setPostponed( DelayTypeSec( PostponedTimeoutSec ) );
                return;
            }
        }
    }

    auto value = block.itemValue().clone();
    if( !value ) {
        m_actionsInfo->setFailed();
        return;
    }

    typename decltype( m_valueToItems )::iterator itr = m_valueToItems.begin(), end = m_valueToItems.end();
    for( ; itr != end; ++itr ) {
        if( std::get<0>( *itr ) == value ) {
            break;
        }
    }

    if( end != itr ) {
        std::get<1>( *itr ).emplace_back( block.itemId() );
    }
    else {
        m_valueToItems.emplace_back( std::move( value ), ItemsList( { block.itemId().c_str() } ) );
    }

    m_actionsInfo->setCompleted();
};

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockSetValueMulticast& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    bool isReachable = true;
    const auto& valToItems = block.valueToItems();
    for( const auto& mapItr : valToItems ) {
        ItemsList itemsList;
        for( const auto& itemId : mapItr.second ) {
            if( !m_actionsInfo->isIgnoreResult() ) {
                if( !isDeviceOfItemReachable( itemId ) ) {
                    if( m_actionsInfo->isPostponed() ) {
                        m_actionsInfo->setFailed();
                        FUNC_LOGW( "%s for itemId=%s failed because device isn't ready!",
                                   block.storKey().c_str(),
                                   itemId.c_str() );
                        return;
                    }

                    isReachable = false;
                    continue;
                }
            }
            itemsList.emplace_back( itemId );
        }
        m_valueToItems.emplace_back( mapItr.first->clone(), std::move( itemsList ) );
    }

    if( m_actionsInfo->isIgnoreResult() ) {
        m_actionsInfo->setCompleted();
    }
    else {
        if( isReachable ) {
            m_actionsInfo->setCompleted();
        }
        else {
            FUNC_LOGI( "%s is postponed", block.storKey().c_str() );
            m_actionsInfo->setPostponed( DelayTypeSec( PostponedTimeoutSec ) );
            return;
        }
    }
}

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockSetValueMulticast2& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    bool isReachable = true;
    const auto& valToItems = block.valueToItems();
    for( const auto& mapItr : valToItems ) {
        ItemsList itemsList;
        for( const auto& itemId : mapItr.second ) {
            if( !m_actionsInfo->isIgnoreResult() ) {
                if( !isDeviceOfItemReachable( itemId ) ) {
                    if( m_actionsInfo->isPostponed() ) {
                        m_actionsInfo->setFailed();
                        FUNC_LOGW( "%s for itemId=%s failed because device isn't ready!",
                                   block.storKey().c_str(),
                                   itemId.c_str() );
                        return;
                    }

                    isReachable = false;
                    continue;
                }
            }
            itemsList.emplace_back( itemId );
        }
        m_valueToItems.emplace_back( mapItr.first.second->clone(), std::move( itemsList ) );
    }

    if( m_actionsInfo->isIgnoreResult() ) {
        m_actionsInfo->setCompleted();
    }
    else {
        if( isReachable ) {
            m_actionsInfo->setCompleted();
        }
        else {
            FUNC_LOGI( "%s is postponed", block.storKey().c_str() );
            m_actionsInfo->setPostponed( DelayTypeSec( PostponedTimeoutSec ) );
            return;
        }
    }
}

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockSetValueMulticast3& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    bool isReachable = true;
    const auto& valToItems = block.valueToItems();
    const auto& indexes = block.indexes();

    for( const auto& idxItr : indexes ) {
        const auto& mapItr = valToItems.find( idxItr );
        if( mapItr == valToItems.end() ) {
            continue;
        }

        ItemsList itemsList;
        for( const auto& itemId : ( *mapItr ).second ) {
            if( !m_actionsInfo->isIgnoreResult() ) {
                if( !isDeviceOfItemReachable( itemId ) ) {
                    if( m_actionsInfo->isPostponed() ) {
                        m_actionsInfo->setFailed();
                        FUNC_LOGW( "%s for itemId=%s failed because device isn't ready!",
                                   block.storKey().c_str(),
                                   itemId.c_str() );
                        return;
                    }

                    isReachable = false;
                    continue;
                }
            }
            itemsList.emplace_back( itemId );
        }
        m_valueToItems.emplace_back( ( *mapItr ).first.second->clone(), std::move( itemsList ) );
    }

    if( m_actionsInfo->isIgnoreResult() ) {
        m_actionsInfo->setCompleted();
    }
    else {
        if( isReachable ) {
            m_actionsInfo->setCompleted();
        }
        else {
            FUNC_LOGI( "%s is postponed", block.storKey().c_str() );
            m_actionsInfo->setPostponed( DelayTypeSec( PostponedTimeoutSec ) );
            return;
        }
    }
}

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockSetDeviceArmed& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    auto& hmMan = core::HouseModesManager::get();
    auto res = hmMan.setDeviceArmed( block.deviceId().c_str(), block.isArmed() );
    if( core::HouseModesManager::Result::OK != res ) {
        FUNC_LOGW( "Method HouseModesManager::setDeviceArmed(deviceId=%s, isArmed=%d)",
                   block.deviceId().c_str(),
                   block.isArmed() );
    }
    m_actionsInfo->setCompleted();
};

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockSendHttpRequest& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    network::HTTPRequest request( block.url(), block.request() );
    request.setCredential( block.user(), block.password() );
    request.setContentType( block.contentType() );
    request.setContent( block.content() );
    request.setHeaders( block.headers() );
    request.setSkipSecurity( block.skipSecurity() );
    request.send();

    m_actionsInfo->setCompleted();
};

#ifdef CONFIG_ADVANCED_SCENES
template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockSwitchHouseMode& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    auto& hmMan = core::HouseModesManager::get();
    if( hmMan.switchPreset( block.houseMode() ) ) {
        m_actionsInfo->setCompleted();
    }
    else {
        m_actionsInfo->setFailed();
    }
}

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockExecuteScene& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    int errorCode;
    std::tie( errorCode, std::ignore ) = m_scenesManager.execScene( block.sceneStorId() );
    if( errors::NotError == errorCode ) {
        m_actionsInfo->setCompleted();
    }
    else {
        m_actionsInfo->setFailed();
    }
}
#endif  // CONFIG_ADVANCED_SCENES

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::visit( const ThenBlockCloudCommand& block ) {
    if( !isDelayExpired( block ) ) {
        return;
    }

    if( m_actionsInfo->isWaitingForConfirmation() ) {
        FUNC_LOGI( "Waiting for confirmation expired!" );
        m_completedIds.erase( block.storKey() );
        m_actionsInfo->setFailed();
    }

    std::shared_ptr<common::IValue> value = block.value()->clone();  // can't use unique_ptr (((
    auto clbk = [cmdId = block.commandId(),
                 capability = block.capability(),
                 command = block.command(),
                 value = value,
                 blockId = block.storKey()]( common::IWriteStream& ws ) -> bool {
        common::JsonStreamGenerator jsonGenerator( ws );

        bool result = jsonGenerator.writeProperty( tokens::Id, blockId );
        result = result && jsonGenerator.writeProperty( tokens::Method, "cloud.controller_abstracts_set" );
        result = result && jsonGenerator.writeProperty( tokens::Api, tokens::CurrentApi );
        result = result && jsonGenerator.writePropertyKey( tokens::Params );
        result = result && jsonGenerator.writeOpeningObject();
        {
            result = result && jsonGenerator.writeProperty( tokens::Uuid, cmdId );
            result = result && jsonGenerator.writeProperty( tokens::Capability, capability );
            result = result && jsonGenerator.writeProperty( tokens::Command, command );
            result = result && jsonGenerator.writeProperty( tokens::Parameters, *value );
        }
        result = result && jsonGenerator.writeClosingSymbol();
        return result && jsonGenerator.writeJsonEnd();
    };

    constexpr auto CmdTimeoutSec = std::chrono::seconds( 1 );
    if( bus::waitToResp<bus::command::nma::ClientClbk>( "nma", CmdTimeoutSec, std::move( clbk ) ) ) {
        m_actionsInfo->setCompleted();
    }
    else {
        m_actionsInfo->setWaitingForConfirmation( DelayTypeSec( PostponedTimeoutSec ) );
        m_completedIds.emplace( block.storKey(), *m_actionsInfo );
    }
}

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::execute( const ThenBlock& thenBlock,
                                                           ActionInfoType* info,
                                                           const TimePointType& prevTp,
                                                           const TimePointType& currTp ) {
    if( info->isCompleted() || info->isFailed() ) {
        return;
    }

    m_actionsInfo = info;
    m_prevTp = prevTp;
    m_currTp = currTp;
    thenBlock.accept( *this );
}

template <typename ScenesManagerType, typename ActionInfoType>
bool Executor<ScenesManagerType, ActionInfoType>::send( bus::IBusClient& busClient ) {
    bool is_success = true;
    auto end = m_valueToItems.end();
    for( auto itr = m_valueToItems.begin(); itr != end; ++itr ) {
        is_success = bus::command::CloudItemValueSet::send(
            "debug", busClient, std::move( std::get<1>( *itr ) ), std::move( std::get<0>( *itr ) ) );
        if( !is_success ) {
            break;
        }
    }
    m_valueToItems.clear();
    return is_success;
}

template <typename ScenesManagerType, typename ActionInfoType>
void Executor<ScenesManagerType, ActionInfoType>::onCompleted( const StringType& id, bool isSuccess ) {
    auto itr = m_completedIds.find( id );
    if( m_completedIds.end() == itr ) {
        return;
    }

    if( isSuccess ) {
        itr->second.setCompleted();
    }
    else {
        itr->second.setFailed();
    }
    m_completedIds.erase( itr );
}

template <typename ScenesManagerType, typename ActionInfoType>
bool Executor<ScenesManagerType, ActionInfoType>::isDelayExpired( const ThenBlock& block ) {
    auto tp = m_actionsInfo->activationTime() + block.delay();
    if( tp > m_currTp ) {
        return false;
    }

    if( tp >= m_prevTp ) {
        return true;
    }

    // Fall there if some time intervals was skipped
    m_actionsInfo->setFailed();
    return false;
}

template <typename ScenesManagerType, typename ActionInfoType>
bool Executor<ScenesManagerType, ActionInfoType>::isDeviceOfItemReachable( const StringType& itemId ) const {
    auto itemInfo = gateways::ItemInfo::create( itemId );
    if( !itemInfo ) {
        FUNC_LOGE( "Can't read ItemInfo from storage for key=%s!", itemId.c_str() );
        return false;
    }

    const auto& deviceId = itemInfo->getDeviceId();
    auto reachable = cache::Cache::get().getValueByKey( deviceId + tokens::Reachable );
    if( !reachable ) {
        return false;
    }

    auto bReachable = common::iValueCast<common::BoolValue>( std::move( reachable ) );
    return bReachable && bReachable->get();
}

}  // end of namespace scenes
