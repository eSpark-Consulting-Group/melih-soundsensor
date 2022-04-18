#pragma once

#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "Bus.h"
#include "BusCommand.h"
#include "BusCommandHandleEvent.h"
#include "IReadStream.h"
#include "NmaBase.h"
#include "NmaWebSocketHandler.h"

#include <memory>
#include <mutex>
#include <chrono>
#include <queue>

namespace gateways {
class IGatewayUserDataStorage;
}

namespace core {
class EspLogger;
namespace features {
class FeatureFlagController;
}
}  // namespace core

namespace scenes {
namespace commands {
class SceneChanged;
}  // namespace commands
}  // namespace scenes

namespace nma {
class Command;
class Parser;
}  // namespace nma

namespace cloud_client {

class CloudNmaClient : public nma::NmaBase {
    friend class CloudNmaClientStartupService;
    static constexpr auto FwUpdatePeriod = std::chrono::hours( 24 );

public:
    using UPtr = std::unique_ptr<CloudNmaClient>;
    using UpdContext = core::upd_srv::UpdContext;

    static constexpr auto InvalidNmaPort = 0;

    static constexpr int SyncProvisioningItemsMaxNumber = 3;

    CloudNmaClient( std::shared_ptr<::core::EspLogger>, ::gateways::IGatewayUserDataStorage& );

    ~CloudNmaClient();

    bool joinToBus() override;

    bool performConnect();

    void updateProvisionDataRequest() override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    void onShutdownStart() override;

    void shutdown() {
        m_socket->close();
    }

#ifdef CONFIG_CLOUD_DEVICES
    bool requestAbstractList();

    bool requestStorageControllerToken();

    template <typename Rep, typename Period>
    void sheduleStorageControllerTokenRequest( const std::chrono::duration<Rep, Period>& delay ) {
        m_isStorageControllerTokenRequest = false;
        m_storageControllerTokenRequestTimepoint = ClockType::now() + delay;
    }
#endif  // CONFIG_CLOUD_DEVICES

    void getStorageFileList( const char* storageType );

    static auto nmaServer() {
        return m_nmaServer;
    }

    static auto nmaPort() {
        return m_nmaPort;
    }

    static bool isValidServer() {
        return !nmaServer().empty() && InvalidNmaPort != nmaPort();
    }

    static void setServerInfo( const std::string nmaSrvName,
                               const unsigned int nmaSrvPort,
                               const bool isConnectionSecured ) {
        m_nmaServer = nmaSrvName;
        m_nmaPort = nmaSrvPort;
        m_isConnectionSecured = isConnectionSecured;
    }

    static bool startCheckProvisionTask();

    static network::ISocket::UPtr createSocket();

    void onRegistered();

    void onFirmwareUpdateCompleted() {
        m_isFirmwareUpdate = false;
    }

    bool tryStartWifiAccessPointsScan();

    bool tryStopWifiAccessPointsScan();

    bool isWifiScanActive() const {
        return TimePoint{} < m_wifiScanTimepoint;
    }

protected:
    // IBusCommandVisitor
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::nma::ServerRequest& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::nma::ClientClbk& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::LocalModeFeatureStatusUpdate& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::CloudDevicesFeatureStatusUpdate& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::AdvancedScenesFeatureStatusUpdate& command ) override;

    void visit(::bus::BusAddr, ::bus::BusAddr, const ::gateways::commands::ProvisioningListEntryAdded& ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, ::bus::command::CreateChipBackup& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, ::bus::command::RestoreChipBackup& command ) override;

    void visit(::bus::BusAddr, ::bus::BusAddr, const ::gateways::commands::ProvisioningListEntryUpdated& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, const ::gateways::commands::ProvisioningListEntryRemoved& ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, const commands::HubProvisionChanged& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const network::wifi_srv2::responce::DiscoveredWifiAccessPoints& cmd ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const commands::FirmwareUpdateCompleted& command ) override {
        m_isFirmwareUpdate = false;
    }

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const commands::ProvisionSyncingCompleted& command ) override {
        m_isCheckProvisioning = false;
    }

private:
    bool initializeImpl() override;

    bool connectToNma( const std::string& srvName, const unsigned int srvPort );

    void onConnected();

    void onConnLost();

    bool tryConnectToNma();

    bool getNewNmaAddress();

    bool processNewServerRequest();

    bool sendFirmwareUpdateRequest();

    bool sendRegisterHubRequest();

    inline void setRestartStartupService() {
        m_isRestartStartupService = true;
    }

    void closeConnection();

    static std::vector<char> getUTCTimeStr();

private:
    bool m_isConnected = false;

    TimePoint m_checkProvisionTimepoint;
    bool m_isCheckProvisioning = false;

    static std::string m_nmaServer;
    static unsigned int m_nmaPort;
    static bool m_isConnectionSecured;
    static bool m_isNewServerRequest;
    static int m_syncProvisionDataAttempCount;

    TimePoint m_fwUpdateTimepoint;
    bool m_isFirmwareUpdate = false;

    bool m_isRestartStartupService = false;
    bool m_isNeedReconnection = false;

    TimePoint m_wifiScanTimepoint;

#ifdef CONFIG_CLOUD_DEVICES
    bool m_isStorageControllerTokenRequest = false;
    std::optional<TimePoint> m_storageControllerTokenRequestTimepoint;
#endif  // CONFIG_CLOUD_DEVICES
};

}  // namespace cloud_client

#endif  // not CONFIG_UART_API
