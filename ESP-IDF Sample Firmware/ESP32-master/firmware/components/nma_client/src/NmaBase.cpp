#include "NmaBase.h"

#include <NmaCommand.h>
#include <NmaWebSocketHandler.h>

#include <Storage.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <ObjectSerializer.h>

#include <HTTPData/HTTPText.h>
#include <CommonResources.h>
#include <CommonResources.h>
#include <GatewayUtils.h>

#include <FirmwareVersion.h>
#include <Utils.h>
#include <HardUtils.h>
#include <Ezlo.h>
#include <IDInfo.h>

#include "lua/responce/AddDevice.h"
#include "lua/responce/UpdateDevice.h"
#include "lua/responce/RemoveDevice.h"
#include "lua/responce/AddItem.h"
#include "lua/responce/UpdateItem.h"
#include "lua/responce/UpdateItemDictionary.h"
#include <BusCommandRemoveItem.h>
#include <BusCommandUpdateGateway.h>
#include "lua/responce/HubSendBroadcast.h"
#include <BusCommandModesAlarmsOffDefault.h>
#include <BusCommandModesAlarmsOffAdded.h>
#include <BusCommandModesAlarmsOffRemoved.h>
#include <BusCommandModesChanged.h>
#include <BusCommandModesDisarmedDevicesDefault.h>
#include <BusCommandModesDisarmedDevicesAdded.h>
#include <BusCommandModesDisarmedDevicesRemoved.h>
#include <BusCommandModesNotificationsNotifyAll.h>
#include <BusCommandModesNotificationsAdded.h>
#include <BusCommandModesNotificationsRemoved.h>
#include <BusCommandModesSwitched.h>

#include <BusCommandNmaRequestTimeZones.h>
#include <BusCommandRoomsCreated.h>
#include <BusCommandRoomsDeleted.h>
#include <BusCommandRoomsReordered.h>
#if CONFIG_REMOTE_LOGGING
#ifdef IS_SAVE_CRASH_DUMP
#include <BusCommandSendCrashLog.h>
#endif  // IS_SAVE_CRASH_DUMP
#endif  // CONFIG_REMOTE_LOGGING
#include <BusCommandRemoteLog.h>
#include "BusCommandFavoriteUpdated.h"
#include "BusCommandNmaClientClbk.h"
#include "nma/responces/SoftwareUpdateProgress.h"

#include "scenes/commands/JsonifyScene.h"

#include "scenes/responces/SceneAdded.h"
#include "scenes/responces/GlobalSceneAdded.h"
#include "scenes/responces/SceneChanged.h"
#include "scenes/responces/GlobalSceneChanged.h"
#include "scenes/responces/SceneDeleted.h"
#include "scenes/responces/GlobalSceneDeleted.h"
#include "scenes/responces/SceneRunProgress.h"
#include "scenes/responces/GlobalSceneRunProgress.h"

#include "BusCommandNmaClientClbk.h"
#include "settings/responces/SettingAdded.h"
#include "settings/responces/SettingRemoved.h"
#include "settings/responces/SettingValueSetted.h"
#include "settings/responces/SettingUpdated.h"
#include "settings/responces/SettingStatusUpdated.h"
#include "settings/responces/SettingDictionaryValueUpdated.h"
#include "settings/responces/GatewaySettingValueSetted.h"
#include "settings/responces/SettingDictionaryStatusUpdated.hpp"
#include "settings/responces/GatewaySettingAdded.h"
#include "settings/responces/GatewaySettingRemoved.h"
#include "settings/responces/GatewaySettingStatusUpdated.h"

#include "dev_dis/DevDiscovery.h"
#include "dev_dis/Commands.h"

#include <values/BoolValue.h>
#include <IValueCast.h>

#include "TCPSocket.h"
#include "SSLSocket.h"

#include <memory>
#include <chrono>
#include <thread>

#include <NmaBroadcastsSender.h>
#include <Cache.h>
#include <IDInfoManager.h>

#include <RemoteLogger.h>
#include <MemoryMappedFlash.h>
#include <FlashUtils.h>
#include <mbedtls/base64.h>

#include <values/TableValue.h>
#include <values/StringValue.h>
#include "HouseModesManager.h"
#include "ItemsUpdatdBroadcastManager.h"

#include "ZWaveZNet.h"

#define ENABLE_NMA_LOG
#ifdef ENABLE_NMA_LOG
constexpr char NMA_CLIENT_TAG[] = "NMA";
#define NMA_LOGE( ... ) ESP_LOGE( NMA_CLIENT_TAG, __VA_ARGS__ )
#define NMA_LOGW( ... ) ESP_LOGW( NMA_CLIENT_TAG, __VA_ARGS__ )
#define NMA_LOGI( ... ) ESP_LOGI( NMA_CLIENT_TAG, __VA_ARGS__ )
#define NMA_LOGD( ... ) ESP_LOGD( NMA_CLIENT_TAG, __VA_ARGS__ )

#else

#define NMA_LOGE( ... ) \
    do {                \
    } while( 0 )
#define NMA_LOGW( ... ) \
    do {                \
    } while( 0 )
#define NMA_LOGI( ... ) \
    do {                \
    } while( 0 )
#define NMA_LOGD( ... ) \
    do {                \
    } while( 0 )

#endif

namespace nma {
namespace {

using namespace common::resources;

inline bool writeServiceNotification( const std::string& deviceId, common::JsonStreamGenerator& gen ) {
    bool isServiceNotification = common::isServiceNotification( deviceId );
    return gen.writeProperty( tokens::ServiceNotification, isServiceNotification );
}

}  // namespace

NmaBase::NmaBase( network::ISocket::UPtr&& sock,
                  std::shared_ptr<::core::EspLogger> logger,
                  ::gateways::IGatewayUserDataStorage& stor )
: m_espLogger{ logger }
, m_gatewayUserDataStorage{ stor }
, m_socket{ std::move( sock ) }
, m_readStream{ std::make_unique<network::RSockStream>( *m_socket ) } {}

int NmaBase::updateDefferedTask() {
    if( !hasDefferedTask() ) {
        return errors::NotError;
    }

    m_isSuccess = true;
    auto [errorCode, errorMessage] = m_taskClbk( m_context->jsGen );
    if( !m_isSuccess ) {
        return errors::InternalError;
    }
    else if( errors::Waiting == errorCode ) {
        return errors::Waiting;
    }

    m_taskClbk = nullptr;
    m_context = nullptr;
    return errorCode;
}

int NmaBase::updateAllDefferedTasks() {
    auto res = jsonHandler().updateDefferedTask();
    if( errors::Waiting == res ) {
        return errors::Waiting;
    }
    else if( errors::NotError != res ) {
        return res;
    }

    res = updateDefferedTask();
    if( errors::Waiting == res ) {
        return errors::Waiting;
    }
    else if( errors::NotError != res ) {
        return res;
    }

    return errors::NotError;
}

NmaBase::ClbkRet NmaBase::update( UpdContext& ctx ) {
    if( ClbkRet::Completed == updateShutdownState() ) {
        return ClbkRet::Completed;
    }

    {
        auto res = updateAllDefferedTasks();
        if( errors::Waiting == res ) {
            return ClbkRet::Continue;
        }
        else if( errors::GenJsonFailed == res ) {
            return ClbkRet::Completed;
        }
    }
    ctx.tryYield();

    m_isSuccess = true;
    while( tryReceiveSingleMessage() ) {
        if( !m_isSuccess ) {
            return ClbkRet::Completed;
        }
        else if( hasDefferedTask() ) {
            return ClbkRet::Continue;
        }
        else if( ctx.isYield() ) {
            ctx.yield();
            break;
        }
    }

    return core::upd_srv::IUpdatable::update( ctx );
}

NmaBase::ClbkRet NmaBase::updateComponent( UpdContext& ctx ) {
    const auto res = jsonHandler().parse( *m_readStream, writeStream() ) ? ClbkRet::Continue : ClbkRet::Completed;
    if( jsonHandler().isNewHandler() ) {
        setJsonHandler( std::move( jsonHandler().getNewHandler() ) );
    }
    return res;
}

void NmaBase::updateProvisionDataRequest() {
    NMA_LOGE( "Not implemented" );
}

bool NmaBase::joinToBus() {
    auto& busInst = bus::Bus::get();
    return busInst.join( "nma", *this ) && busInst.join( lua::responce::BUS_NAME, *this ) &&
           busInst.join( dev_dis::responce::BUS_NAME, *this );
}

NmaBase::WStreamPtr NmaBase::writeStream() {
    return std::make_unique<network::WSockStream>( *m_socket );
}

bool NmaBase::canSendItemUpdatedRequestForItem( const gateways::ItemInfo& itemInfo ) {
    return ItemsUpdatdBroadcastManager::get().canSendItemUpdate( itemInfo );
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::UpdateGateway& command ) {
    ::nma::BroadcastsSender::sendGatewayUpdated( command.getGatewayId(), std::move( command.getUpdatedFields() ) );
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddDevice& command ) {
    common::IValue::UPtr value = std::make_unique<common::BoolValue>( true );
    if( command.m_deviceInfo ) {
        cache::Cache::get().setValueByKey( std::string( command.m_deviceInfo->getId() ) + tokens::Reachable,
                                           std::move( value ) );
    }

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceAdded );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && gateways::utils::writeDeviceInfo( jsGen, *command.m_deviceInfo.get() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::RemoveDevice& command ) {
    if( command.m_deviceId.empty() ) {
        return;
    }

    cache::Cache::get().remove( command.m_deviceId + tokens::Reachable );

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceRemoved );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SubId, command.m_deviceId );
        m_isSuccess =
            m_isSuccess && jsGen.writeProperty( tokens::ServiceNotification, command.m_isServiceNotification );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    }
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateDevice& command ) {
    if( command.m_deviceId.empty() || !command.m_value ) {
        return;
    }

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();

    bool isArmed = core::HouseModesManager::get().isDeviceArmed( command.m_deviceId );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::DeviceArmed, isArmed );

    m_isSuccess = m_isSuccess && gateways::utils::writeUpdatedField(
                                     jsGen, command.m_deviceId.c_str(), command.m_key.c_str(), command.m_value.get() );

    m_isSuccess = m_isSuccess && writeServiceNotification( command.m_deviceId, jsGen );

    if( strncmp( tokens::Name, command.m_key.c_str(), strlen( tokens::Name ) ) == 0 ||
        strncmp( tokens::RoomId, command.m_key.c_str(), strlen( tokens::RoomId ) ) == 0 ) {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    }
    else {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, false );
    }
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();

    if( command.m_key == tokens::Ready ) {
        std::string key = command.m_deviceId.c_str() + command.m_key;
        cache::Cache::get().setValueByKey( key, std::move( command.m_value ) );
    }
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddItem& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubItemAdded );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && gateways::utils::writeItemInfo( jsGen, *command.m_itemInfo.get() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, settings::responces::SettingAdded& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceSettingAdded );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && gateways::utils::writeSettingInfo( jsGen, *command.m_settingInfo.get() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     settings::responces::GatewaySettingAdded& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubGatewaySettingAdded );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && gateways::utils::writeGatewaySettingsInfo( jsGen, *command.m_settingInfo.get() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, settings::responces::SettingRemoved& command ) {
    if( command.m_settingId.empty() ) {
        return;
    }

    cache::Cache::get().remove( command.m_settingId );

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceSettingRemoved );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SubId, command.m_settingId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     settings::responces::GatewaySettingRemoved& command ) {
    if( command.m_settingId.empty() ) {
        return;
    }

    cache::Cache::get().remove( command.m_settingId );

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubGatewaySettingRemoved );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SubId, command.m_settingId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     settings::responces::SettingValueSetted& command ) {
    if( !command.m_settingInfo ) {
        return;
    }

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceSettingUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Params );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && gateways::utils::writeSettingUpdatedInfo(
                                     jsGen, *command.m_settingInfo.get(), command.m_value.get() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, false );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, settings::responces::SettingUpdated& command ) {
    if( !command.m_setting ) {
        return;
    }

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceSettingUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Result, *command.m_setting );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     settings::responces::GatewaySettingValueSetted& command ) {
    if( !command.m_settingInfo ) {
        return;
    }

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubGatewaySettingUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Params );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && gateways::utils::writeGatewaySettingsUpdatedInfo(
                                     jsGen, *command.m_settingInfo.get(), command.m_value.get() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, false );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     settings::responces::SettingStatusUpdated& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceSettingUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Params );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess &&
                  gateways::utils::writeSettingStatusUpdatedInfo( jsGen, command.getSettingId(), command.getStatus() );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     settings::responces::GatewaySettingStatusUpdated& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubGatewaySettingUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Params );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess &&
                  gateways::utils::writeSettingStatusUpdatedInfo( jsGen, command.getSettingId(), command.getStatus() );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     settings::responces::SettingDictionaryValueUpdated& command ) {
    if( !command.getSettingInfo() ) {
        return;
    }

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess =
        m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubDeviceSettingDictionaryUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess &&
                  gateways::utils::writeSettingDictionaryUpdatedInfo(
                      jsGen, *command.getSettingInfo(), command.getOperation(), command.getKey(), command.getValue() );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit(::bus::BusAddr, ::bus::BusAddr, ::settings::responces::SettingDictionaryStatusUpdated& command ) {
    ::nma::BroadcastsSender::sendSettingDictionaryStatusUpdated(
        command.getSettingId(), command.getElementId(), command.getStatus() );
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RemoveItem& command ) {
    if( command.m_itemId.empty() ) {
        return;
    }
    cache::Cache::get().remove( command.m_itemId );

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubItemRemoved );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SubId, command.m_itemId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::DeviceId, command.m_deviceId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateItem& command ) {
    if( !command.m_itemInfo || !command.isValueValid() ) {
        return;
    }

    if( command.m_itemInfo->isUserLockOperation() ) {
        std::string name;
        const auto tableValue = common::iValueCast<common::TableValue>( std::move( command.value() ) );

        const auto action = common::iValueCast<common::StringValue>( ( *tableValue )[tokens::Action] );
        if( action->get() != "unknown" ) {
            const auto userId = common::iValueCast<common::IntValue>( ( *tableValue )[tokens::UserId] );
            if( userId ) {
                auto userCode = command.m_itemInfo->getUserLockNameCode( userId->get() );
                if( userCode ) {
                    name = userCode->getName();
                }
                else {
                    char id[10];
                    sprintf( id, "Code %d", userId->get() );
                    name.assign( id );
                }
            }

            const auto& deviceId = command.m_itemInfo->getDeviceId();
            gateways::enumerateDevicesFromDb( [this, deviceId, name]( const gateways::BaseDeviceInfo& deviceInfo ) {
                if( deviceInfo.getId() == deviceId ) {
                    const auto ws = writeStream();
                    if( ws ) {
                        common::JsonStreamGenerator jsGen( *ws );

                        auto m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
                        m_isSuccess =
                            m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubUserNotification );
                        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
                        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
                        m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
                        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Type, tokens::PinCodeIsUsed );
                        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Params );
                        m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
                        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::User, name );
                        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::DeviceName, deviceInfo.getName() );
                        auto pagesManager = core::PagesManager::get();
                        if( pagesManager ) {
                            const auto& pageName = pagesManager->getPageName( deviceInfo.getRoomId() );
                            m_isSuccess = m_isSuccess && jsGen.writeProperty( broadcasts::RoomName, pageName );
                        }
                        else {
                            m_isSuccess = false;
                        }
                        m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
                        m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
                        m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
                        m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();

                        assert( m_isSuccess );
                    }
                    return false;
                }
                return true;
            } );
        }
    }

    auto value = command.value();

    std::string delayCacheValue = "updated_item_id_" + command.m_itemInfo.get()->getId();

    auto broadcaseDelayCacheValue = cache::Cache::get().getValueByKey( delayCacheValue.c_str() );
    if( ( broadcaseDelayCacheValue && ( *value == *broadcaseDelayCacheValue ) ) ) {
        return;
    }

    if( canSendItemUpdatedRequestForItem( *command.m_itemInfo.get() ) ) {
        if( !broadcaseDelayCacheValue || ( *value != *broadcaseDelayCacheValue ) ) {
            cache::Cache::get().setValueByKey( delayCacheValue.c_str(), value->clone() );

            auto ws = writeStream();
            if( !ws ) {
                return;
            }
            common::JsonStreamGenerator jsGen( *ws );

            m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubItemUpdated );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
            m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
            m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
            m_isSuccess =
                m_isSuccess && gateways::utils::writeItemUpdatedInfo( jsGen, *command.m_itemInfo.get(), value.get() );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, false );
            m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
            m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
        }
    }
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateItemDictionary& command ) {
    if( !command.m_itemInfo ) {
        return;
    }

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubItemDictionaryUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess =
        m_isSuccess &&
        gateways::utils::writeItemDictionaryUpdatedInfo(
            jsGen, *command.m_itemInfo.get(), command.m_operation, command.m_itemDictionaryId, command.m_value.get() );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesSwitched& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.switched" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::From, command.m_from );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::To, command.m_to );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Status, command.m_status );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SwitchToDelay, command.m_switchDelay );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     bus::command::ModesNotificationsNotifyAll& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.notifications.notify_all" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::ModeId, command.m_modeId );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::All, command.m_all );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesNotificationsAdded& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.notifications.added" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::ModeId, command.m_modeId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Notification, command.m_notification.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     bus::command::ModesNotificationsRemoved& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.notifications.removed" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::ModeId, command.m_modeId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Notification, command.m_notification.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesChanged& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.changed" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::ModeId, command.m_modeId.c_str() );
    for( const auto& property : command.m_properties ) {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( property.first.c_str(), *property.second );
    }
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     bus::command::ModesDisarmedDevicesDefault& command ) {
    /// TODO: need create API broadcast and implement it
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     bus::command::ModesDisarmedDevicesAdded& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.disarmed_devices.added" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::ModeId, command.m_modeId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::DisarmedDevice, command.m_disarmedDevice.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     bus::command::ModesDisarmedDevicesRemoved& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.disarmed_devices.removed" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::ModeId, command.m_modeId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::DisarmedDevice, command.m_disarmedDevice.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesAlarmsOffDefault& command ) {
    /// TODO: need create API broadcast and implement it
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesAlarmsOffAdded& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.alarms_off.added" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::ModeId, command.m_modeId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::AlarmsOffDevice, command.m_alarmOff.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesAlarmsOffRemoved& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.modes.alarms_off.removed" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::ModeId, command.m_modeId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::AlarmsOffDevice, command.m_alarmOff.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, const scenes::responces::SceneAdded& command ) {
    auto context = std::make_unique<Context>( writeStream() );

    if( !context->jsGen.writeProperty( tokens::Id, broadcasts::Id ) ||
        !context->jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubSceneAdded ) ||
        !context->jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() ) ||
        !context->jsGen.writePropertyKey( tokens::Result ) ) {
        return;
    }

    auto resp = asyncSend<scenes::commands::JsonifyScene>(
        scenes::commands::LOCAL_BUS_NAME, context->jsGen, command.sceneId() );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( common::JsonStreamGenerator& gen ) -> ResponceType {
        if( std::future_status::ready != asyncResp.wait_for( std::chrono::milliseconds( 0 ) ) ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }
        auto [errorCode, errorDescr] = asyncResp.get();
        if( errors::GenJsonFailed != errorCode ) {
            gen.writeJsonEnd();
        }
        return std::make_tuple( errorCode, std::move( errorDescr ) );
    };

    m_taskClbk = std::move( clbk );
    m_context = std::move( context );
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     const scenes::responces::GlobalSceneAdded& command ) {
    auto context = std::make_unique<Context>( writeStream() );

    if( !context->jsGen.writeProperty( tokens::Id, broadcasts::Id ) ||
        !context->jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubGlobalSceneAdded ) ||
        !context->jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() ) ||
        !context->jsGen.writePropertyKey( tokens::Result ) ) {
        return;
    }

    auto resp = asyncSend<scenes::commands::JsonifyScene>(
        scenes::commands::GLOBAL_BUS_NAME, context->jsGen, command.sceneId() );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( common::JsonStreamGenerator& gen ) -> ResponceType {
        if( !asyncResp.valid() ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }
        auto [errorCode, errorDescr] = asyncResp.get();
        if( errors::GenJsonFailed != errorCode ) {
            gen.writeJsonEnd();
        }
        return std::make_tuple( errorCode, std::move( errorDescr ) );
    };

    m_taskClbk = std::move( clbk );
    m_context = std::move( context );
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, const scenes::responces::SceneChanged& command ) {
    auto context = std::make_unique<Context>( writeStream() );

    if( !context->jsGen.writeProperty( tokens::Id, broadcasts::Id ) ||
        !context->jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubSceneChanged ) ||
        !context->jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() ) ||
        !context->jsGen.writePropertyKey( tokens::Result ) ) {
        return;
    }

    auto resp = asyncSend<scenes::commands::JsonifyScene>(
        scenes::commands::LOCAL_BUS_NAME, context->jsGen, command.sceneId() );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( common::JsonStreamGenerator& gen ) -> ResponceType {
        if( !asyncResp.valid() ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }
        auto [errorCode, errorDescr] = asyncResp.get();
        if( errors::GenJsonFailed != errorCode ) {
            gen.writeJsonEnd();
        }
        return std::make_tuple( errorCode, std::move( errorDescr ) );
    };

    m_taskClbk = std::move( clbk );
    m_context = std::move( context );
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     const scenes::responces::GlobalSceneChanged& command ) {
    auto context = std::make_unique<Context>( writeStream() );

    if( !context->jsGen.writeProperty( tokens::Id, broadcasts::Id ) ||
        !context->jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubSceneGlobalChanged ) ||
        !context->jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() ) ||
        !context->jsGen.writePropertyKey( tokens::Result ) ) {
        return;
    }

    auto resp = asyncSend<scenes::commands::JsonifyScene>(
        scenes::commands::GLOBAL_BUS_NAME, context->jsGen, command.sceneId() );
    auto asyncResp = std::shared_future( std::move( resp ) );
    auto clbk = [asyncResp]( common::JsonStreamGenerator& gen ) -> ResponceType {
        if( !asyncResp.valid() ) {
            return std::make_tuple( errors::Waiting, StringType() );
        }
        auto [errorCode, errorDescr] = asyncResp.get();
        if( errors::GenJsonFailed != errorCode ) {
            gen.writeJsonEnd();
        }
        return std::make_tuple( errorCode, std::move( errorDescr ) );
    };

    m_taskClbk = std::move( clbk );
    m_context = std::move( context );
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, const scenes::responces::SceneDeleted& command ) {
    auto wStream = writeStream();
    common::JsonStreamGenerator jsGen( *wStream );
    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubSceneDeleted );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SubId, command.sceneId() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     const scenes::responces::GlobalSceneDeleted& command ) {
    auto wStream = writeStream();
    common::JsonStreamGenerator jsGen( *wStream );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubGlobalSceneDeleted );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SubId, command.sceneId() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SyncNotification, true );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     const scenes::responces::SceneRunProgress& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::SceneRunProgress );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();  // {

    m_isSuccess = m_isSuccess && jsGen.writeProperty( broadcasts::SceneId, command.sceneId() );

    if( m_isSuccess && !command.sceneName().empty() ) {
        m_isSuccess = jsGen.writeProperty( tokens::SceneName, command.sceneName() );
    }
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Status, command.status() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::UserNotification, command.isUserNotifications() );

    if( command.isUserNotifications() ) {
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( broadcasts::Notifications );
        m_isSuccess = m_isSuccess && jsGen.writeOpeningArray();  // [
        for( const auto& next_notification : command.notificationsList() ) {
            m_isSuccess = m_isSuccess && jsGen.writeProperty( nullptr, next_notification );
        }
        m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();  // ]
    }
    else {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( broadcasts::Notifications, nullptr );
    }
    m_isSuccess = m_isSuccess && jsGen.writeProperty( broadcasts::RoomId, command.roomId() );
    {
        auto pagesManager = core::PagesManager::get();
        if( pagesManager ) {
            auto pageName = pagesManager->getPageName( command.roomId() );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( broadcasts::RoomName, pageName );
        }
        else {
            m_isSuccess = false;
        }
    }
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();  // }
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     const scenes::responces::GlobalSceneRunProgress& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SceneName2, command.sceneName() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::SceneGlobalRunProgress );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SceneId, command.sceneId() );
        if( m_isSuccess && !command.tag().empty() ) {
            m_isSuccess = jsGen.writeProperty( tokens::Tag, command.tag() );
        }
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Status, command.status() );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Error, nullptr );
    }
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::HubSendBroadcast& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    if( strcmp( command.m_event.c_str(), common::resources::broadcasts::events::NodeFirmwareDownloadStatus ) == 0 ) {
        m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
        m_isSuccess =
            m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass,
                                                common::resources::broadcasts::HubDeviceFirmwareDownloadProgress );

        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
        m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Image );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[0].c_str() );
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Checksum );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[1].c_str() );
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Status );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[2].c_str() );
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Progress );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[3].c_str() );
    }
    else if( strcmp( command.m_event.c_str(), common::resources::broadcasts::events::NodeFirmwareUpdateStatus ) == 0 ) {
        m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
        m_isSuccess =
            m_isSuccess &&
            jsGen.writeProperty( tokens::MsgSubclass, common::resources::broadcasts::HubDeviceFirmwareUpdateProgress );

        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
        m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( "device_id" );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[0].c_str() );
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::FirmwareId );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[1].c_str() );
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Status );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[2].c_str() );
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Progress );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[3].c_str() );

        const auto image = gateways::zwave::ZNet::get().getUpdateImageName();

        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Image );
        m_isSuccess = m_isSuccess && jsGen.writePropertyValue( image.c_str() );
    }
    else {
        m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.extensions.plugin.ui_broadcast" );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
        m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Event, command.m_event.c_str() );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Plugin, command.m_plugin.c_str() );

        if( command.m_value.size() ) {
            FUNC_LOGI( " %s", command.m_event.c_str() );

            if( strcmp( command.m_event.c_str(), broadcasts::events::IncludeDeviceProgress ) == 0 ) {
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Value );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[0].c_str() );
            }
            else if( strcmp( command.m_event.c_str(), broadcasts::events::LearnProgress ) == 0 ||
                     strcmp( command.m_event.c_str(), broadcasts::events::ReconstructProgress ) == 0 ) {
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Devices );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[0].c_str() );
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Current );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[1].c_str() );
            }
            else if( strcmp( command.m_event.c_str(), broadcasts::events::ReconstructProgressError ) == 0 ) {
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::ZwaveNode );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[0].c_str() );
            }
            else if( strcmp( command.m_event.c_str(), broadcasts::events::RediscoverDeviceProgress ) == 0 ) {
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Progress );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[0].c_str() );
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Status );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[1].c_str() );
            }
            else if( strcmp( command.m_event.c_str(), broadcasts::events::ModuleReset ) == 0 ) {
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Status );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[0].c_str() );
            }
            else if( strcmp( command.m_event.c_str(), broadcasts::events::NodeFirmwareDownloadStatus ) == 0 ) {
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Image );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[0].c_str() );
                m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Status );
                m_isSuccess = m_isSuccess && jsGen.writePropertyValue( command.m_value[1].c_str() );
            }

            else {
                if( strcmp( command.m_event.c_str(), broadcasts::events::S2SelectAuthenticationModes ) == 0 ) {
                    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Modes );
                }
                else if( strcmp( command.m_event.c_str(), broadcasts::events::S2RequestDeviceSpecificKey ) == 0 ||
                         strcmp( command.m_event.c_str(), broadcasts::events::S2ShowDeviceSideKey ) == 0 ||
                         strcmp( command.m_event.c_str(), broadcasts::events::LearnShowKey ) == 0 ) {
                    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Key );
                }
                m_isSuccess = m_isSuccess && jsGen.writeOpeningArray();
                bool first = true;
                for( std::string mode : command.m_value ) {
                    if( !first ) {
                        m_isSuccess = m_isSuccess && jsGen.writeItemDelimiter();
                    }
                    m_isSuccess = m_isSuccess && jsGen.writePropertyValue( mode.c_str() );
                    first = false;
                }
                m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
            }
        }
    }

    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from,
                     const bus::BusAddr to,
                     nma::responces::HubSoftwareUpdateProgress& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }

    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubSoftwareUpdateProgress );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    {
        if( command.isAction() ) {
            m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Action );
            m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
            {
                m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Operation, command.actionOperationString() );
                m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Type, command.actionTypeString() );
                m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Id, command.actionId() );
                m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Version, command.actionVersion() );
            }
            m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
        }

        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Completed, command.operationCompletedProgress() );
        if( command.isError() ) {
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Error, errors::ApplicationError );
        }
        else {
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Error, nullptr );
        }

        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::OperationId, command.operationId() );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Status, command.statusString() );
    }
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol() && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::nma::RequestTimeZones& command ) {
    using namespace common::resources;

    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    std::array<char, 32> idBuffer;

    sprintf( idBuffer.data(),
             "AT32-%" PRIX32 "-%" PRIX32,
             hard::common::generateUniqueId(),
             hard::common::generateUniqueId() );

    m_isSuccess = jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Id, idBuffer.data() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Method, commands::CloudTimeZonesGet );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Params );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Timezone, command.timezoneId() );
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

#if CONFIG_REMOTE_LOGGING
#ifdef IS_SAVE_CRASH_DUMP
void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SendCrashLog& command ) {
    auto logger = core::Logger::get();
    if( !logger || !logger->isAllowed() ) {
        return;
    }

    hard::MemoryMappedFlash crash_dump_vol;
    if( !crash_dump_vol.map_partition( "nvs_ext", hard::MemoryMappedFlash::DATA_TYPE ) ) {
        FUNC_LOGE( "Can't map crash dump partition" );
        return;
    }
    const unsigned char* dump_begin = (const unsigned char*)crash_dump_vol.get();
    constexpr std::uint32_t COREDUMP_FLASH_MAGIC_START = 0xE32C04EDU;
    if( *(std::uint32_t*)dump_begin != COREDUMP_FLASH_MAGIC_START ) {
        FUNC_LOGI( "No crash dump" );
        return;
    }
    constexpr auto MAX_DUMP_SIZE = 64 * 1024;
    auto dump_end = dump_begin + 4;
    while( dump_end < dump_begin + MAX_DUMP_SIZE ) {
        if( *(std::uint32_t*)dump_end == COREDUMP_FLASH_MAGIC_START ) {
            break;
        }
        dump_end += 4;
    }
    if( dump_end > dump_begin + MAX_DUMP_SIZE ) {
        FUNC_LOGE( "Crash dump is corrupted" );
        if( !hard::eraseFlash( crash_dump_vol.get_flash_offset(), MAX_DUMP_SIZE ) ) {
            FUNC_LOGE( "Failed to erase crash dump" );
        }
        return;
    }
    dump_end += 4;

    auto ws = writeStream();
    if( !ws ) {
        FUNC_LOGE( "Failed to create NMA write stream" );
        return;
    }

    common::JsonStreamGenerator jsGen( *ws );
    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.log" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();

    m_isSuccess = m_isSuccess && jsGen.writeProperty( "tag", tokens::CrashDump );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( "timestamp", 0 );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( "severity", core::Logger::LevelToText( core::Logger::ERROR ) );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Message );

    m_isSuccess = m_isSuccess && ws->write( "\"", 1 );
    constexpr unsigned int IN_CHUNK_SIZE = 3 * 64U - 3;
    constexpr unsigned int OUT_CHUNK_SIZE = 4 * 64U;
    std::array<char, OUT_CHUNK_SIZE> buff;
    std::size_t next_chunk_size = IN_CHUNK_SIZE;
    while( dump_begin < dump_end ) {
        if( dump_begin + next_chunk_size > dump_end ) {
            next_chunk_size = dump_end - dump_begin;
        }
        std::size_t out_size;
        if( 0 != mbedtls_base64_encode( reinterpret_cast<unsigned char*>( buff.data() ),
                                        OUT_CHUNK_SIZE,
                                        &out_size,
                                        dump_begin,
                                        next_chunk_size ) ) {
            FUNC_LOGE( "Base64 encode error: %d", out_size );
            constexpr char error_message[] = "...ERROR";
            m_isSuccess = m_isSuccess && ws->write( error_message, sizeof( error_message ) - 1 );
        }
        m_isSuccess = m_isSuccess && ws->write( buff.data(), out_size );
        dump_begin += next_chunk_size;
    }
    m_isSuccess = m_isSuccess && ws->write( "\"", 1 );

    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();  // Result
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
    if( !hard::eraseFlash( crash_dump_vol.get_flash_offset(), MAX_DUMP_SIZE ) ) {
        FUNC_LOGE( "Failed to erase crash dump" );
    }
    return;
}
#endif  // IS_SAVE_CRASH_DUMP

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RemoteLog& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );
    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, "hub.log" );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( "tag", command.m_tag );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( "timestamp", command.m_timestamp );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Message, command.m_logText );
        m_isSuccess = m_isSuccess && jsGen.writeProperty( "severity", core::Logger::LevelToText( command.m_logLevel ) );
    }
    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}
#endif

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RoomsCreated& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    if( command.m_edited ) {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubRoomEdited );
    }
    else {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubRoomCreated );
    }
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SubId, command.m_roomId.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Name, command.m_name.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RoomsDeleted& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubRoomDeleted );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::SubId, command.m_deletedRoom.c_str() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RoomsReordered& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, broadcasts::HubRoomReordered );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::RoomsId );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningArray();

    for( std::string room : command.m_reorderedRooms ) {
        m_isSuccess = jsGen.writeProperty( nullptr, room.c_str() );
    }

    m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::FavoriteUpdated& command ) {
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = jsGen.writeProperty( tokens::Id, broadcasts::Id );
    const char* operation = nullptr;
    if( command.m_operation == bus::command::FavoriteUpdated::Operation::added ) {
        operation = broadcasts::HubFavoriteAdded;
    }
    else {
        operation = broadcasts::HubFavoriteRemoved;
    }
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgSubclass, operation );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::MsgId, m_handler->getMsgId() );
    m_isSuccess = m_isSuccess && jsGen.writePropertyKey( tokens::Result );
    m_isSuccess = m_isSuccess && jsGen.writeOpeningObject();

    for( auto& item : command.m_favoriteObject ) {
        m_isSuccess = m_isSuccess && jsGen.writePropertyKey( item.first.c_str() );
        m_isSuccess = m_isSuccess && jsGen.writeOpeningArray();
        for( auto& object : item.second ) {
            m_isSuccess = m_isSuccess && jsGen.writeProperty( NULL, object.c_str() );
        }
        m_isSuccess = m_isSuccess && jsGen.writeClosingSymbol();
    }

    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
bool NmaBase::enableDeviceDetection( const bool isEnable ) {
    auto instance = dev_dis::DevDiscovery::get();
    if( !instance ) {
        return false;
    }

    if( isEnable ) {
        m_isDeviceDetectionEnabled = instance->startUnknowDevDetection();
        return m_isDeviceDetectionEnabled;
    }

    m_isDeviceDetectionEnabled = !instance->stopUnknowDevDetection();
    return !m_isDeviceDetectionEnabled;
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, const dev_dis::responce::DiscoveredDevices& cmd ) {
    // https://confluence.mios.com/display/ELFD/Devices+Detection#DevicesDetection-hub.detection.device.updated
    auto ws = writeStream();
    if( !ws ) {
        return;
    }

    {
        common::JsonStreamGenerator jsGen( *ws );
        const auto& registeredDevices = cmd.registeredState();
        for( const auto& [uuid, state] : registeredDevices ) {
            const auto& [rssi, timepoint] = state;
            m_isSuccess = BroadcastsSender::writeDefaultHeader( jsGen, broadcasts::HubDetectionDeviceUpdated );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Id, uuid.toHexString() );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Rssi, rssi );
            m_isSuccess =
                m_isSuccess && jsGen.writeProperty( tokens::DetectedLastTime, dev_dis::toDateString( timepoint ) );
            m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
            if( !m_isSuccess ) {
                return;
            }
        }
    }

    if( m_isDeviceDetectionEnabled ) {
        common::JsonStreamGenerator jsGen( *ws );
        const auto& unknowDevices = cmd.unknowState();
        for( const auto& [addr, state] : unknowDevices ) {
            const auto& [rssi, timepoint] = state;
            m_isSuccess = BroadcastsSender::writeDefaultHeader( jsGen, broadcasts::HubDetectionDeviceDetecteded );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Mac, addr.toHexString() );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Rssi, rssi );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Protocol, tokens::WifiDevice );
            m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Channel, CONFIG_WIFI_SERVICE_SNIFFER_CHANNEL );
            m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
        }
    }
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, const dev_dis::responce::DeviceAdded& cmd ) {
    // https://confluence.mios.com/display/ELFD/Devices+Detection#DevicesDetection-hub.detection.device.added
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    const auto& info = cmd.deviceInfo();
    m_isSuccess = BroadcastsSender::writeDefaultHeader( jsGen, broadcasts::HubDetectionDeviceAdded );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Id, info.getId().toHexString() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Name, info.getName() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Mac, info.getMac().toHexString() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Precision, info.getPrecision() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Protocol, tokens::WifiDevice );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, const dev_dis::responce::DeviceRemoved& cmd ) {
    // https://confluence.mios.com/display/ELFD/Devices+Detection#DevicesDetection-hub.detection.device.removed
    auto ws = writeStream();
    if( !ws ) {
        return;
    }
    common::JsonStreamGenerator jsGen( *ws );

    m_isSuccess = BroadcastsSender::writeDefaultHeader( jsGen, broadcasts::HubDetectionDeviceRemoved );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Id, cmd.deviceId().toHexString() );
    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
}

void NmaBase::visit( const bus::BusAddr from, const bus::BusAddr to, const dev_dis::responce::DeviceEdited& cmd ) {
    // https://confluence.mios.com/display/ELFD/Devices+Detection#DevicesDetection-hub.detection.device.updated
    auto ws = writeStream();
    if( !ws ) {
        return;
    }

    common::JsonStreamGenerator jsGen( *ws );
    m_isSuccess = BroadcastsSender::writeDefaultHeader( jsGen, broadcasts::HubDetectionDeviceUpdated );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Id, cmd.deviceId().toHexString() );
    m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Mac, cmd.addr().toHexString() );
    if( cmd.name() ) {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Name, *cmd.name() );
    }
    if( cmd.precision() ) {
        m_isSuccess = m_isSuccess && jsGen.writeProperty( tokens::Precision, *cmd.precision() );
    }

    m_isSuccess = m_isSuccess && jsGen.writeJsonEnd();
    if( !m_isSuccess ) {
        return;
    }
}
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

}  // namespace nma
