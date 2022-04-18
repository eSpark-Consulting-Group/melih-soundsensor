#pragma once

#include "Bus.h"
#include "ISocket.h"
#include "INmaJsonHandler.h"
#include "Shutdownable.h"
#include "JsonStreamGenerator.h"
#include <DeviceInfo.h>

#include <memory>
#include <chrono>

namespace core {
class EspLogger;
}  // namespace core

namespace gateways {
class IGatewayUserDataStorage;
}  // namespace gateways

namespace nma {
// class Command;
// class Parser;

class NmaBase : public core::Shutdownable {
public:
    using ParentType = core::Shutdownable;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = ParentType::UpdContext;

    using WStreamPtr = common::IWriteStream::UPtr;
    using RStreamPtr = common::IReadStream::UPtr;

    using StringType = std::string;
    using ResponceType = std::tuple<int, StringType>;
    using TaskClbkType = std::function<ResponceType( common::JsonStreamGenerator& )>;

    struct Context {
        Context( common::IWriteStream::UPtr&& _ws )
        : ws( std::move( _ws ) )
        , jsGen( *ws.get() ) {}

        common::IWriteStream::UPtr ws;
        common::JsonStreamGenerator jsGen;
    };

    NmaBase( network::ISocket::UPtr&& sock,
             std::shared_ptr<::core::EspLogger> logger,
             ::gateways::IGatewayUserDataStorage& stor );

    ClbkRet update( UpdContext& ctx ) override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    virtual bool joinToBus();

    virtual WStreamPtr writeStream();

    virtual void updateProvisionDataRequest();

    inline auto& jsonHandler() {
        return *m_handler;
    }

    inline void setJsonHandler( INmaJsonHandler::UPtr handler ) {
        m_handler = std::move( handler );
    }

    inline bool hasDefferedTask() const {
        return nullptr != m_taskClbk;
    }

    int updateDefferedTask();

    int updateAllDefferedTasks();

    bool canSendItemUpdatedRequestForItem( const gateways::ItemInfo& itemInfo );

    bool enableDeviceDetection( const bool isEnable );

protected:
    // IBusCommandVisitor
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::UpdateGateway& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddDevice& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateDevice& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::RemoveDevice& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddItem& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, settings::responces::SettingAdded& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, settings::responces::SettingRemoved& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::responces::GatewaySettingAdded& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::responces::GatewaySettingRemoved& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::responces::SettingValueSetted& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, settings::responces::SettingUpdated& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::responces::GatewaySettingValueSetted& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::responces::SettingStatusUpdated& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::responces::GatewaySettingStatusUpdated& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::responces::SettingDictionaryValueUpdated& command ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::settings::responces::SettingDictionaryStatusUpdated& ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateItem& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::UpdateItemDictionary& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RemoveItem& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesSwitched& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::ModesNotificationsNotifyAll& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::ModesNotificationsAdded& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::ModesNotificationsRemoved& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesChanged& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::ModesDisarmedDevicesDefault& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::ModesDisarmedDevicesAdded& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::ModesDisarmedDevicesRemoved& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesAlarmsOffDefault& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesAlarmsOffAdded& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesAlarmsOffRemoved& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, const scenes::responces::SceneAdded& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const scenes::responces::GlobalSceneAdded& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const scenes::responces::SceneChanged& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const scenes::responces::GlobalSceneChanged& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const scenes::responces::SceneDeleted& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const scenes::responces::GlobalSceneDeleted& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const scenes::responces::SceneRunProgress& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const scenes::responces::GlobalSceneRunProgress& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::HubSendBroadcast& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::nma::RequestTimeZones& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RoomsCreated& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RoomsDeleted& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RoomsReordered& command ) override;
#if CONFIG_REMOTE_LOGGING
#ifdef IS_SAVE_CRASH_DUMP
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SendCrashLog& command ) override;
#endif  // IS_SAVE_CRASH_DUMP
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RemoteLog& command ) override;
#endif  // CONFIG_REMOTE_LOGGING
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::FavoriteUpdated& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                nma::responces::HubSoftwareUpdateProgress& command ) override;

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const dev_dis::responce::DiscoveredDevices& cmd ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, const dev_dis::responce::DeviceAdded& cmd ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, const dev_dis::responce::DeviceRemoved& cmd ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, const dev_dis::responce::DeviceEdited& cmd ) override;
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

protected:
    std::shared_ptr<::core::EspLogger> m_espLogger;
    ::gateways::IGatewayUserDataStorage& m_gatewayUserDataStorage;

    network::ISocket::UPtr m_socket;
    RStreamPtr m_readStream;

    INmaJsonHandler::UPtr m_handler;
    bool m_isSuccess;

    TaskClbkType m_taskClbk;
    std::unique_ptr<Context> m_context;
#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
    bool m_isDeviceDetectionEnabled{ false };
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
};

}  // namespace nma
