#pragma once

#include "Bus.h"
#include "BusCommand.h"
#include "BusCommandHandleEvent.h"
#include "IReadStream.h"
#include "NmaBase.h"
#include "NmaWebSocketHandler.h"
#include "BusEventsHandler.h"

#include <memory>
#include <mutex>
#include <chrono>
#include <queue>

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

namespace ucloud_client {

class UCloudNmaClient : public nma::NmaBase {
    constexpr static int CONNECT_WAIT_TIMEOUT_MS = 5000;

    static constexpr auto FwUpdatePeriod = std::chrono::hours( 24 );

public:
    UCloudNmaClient( std::shared_ptr<::core::EspLogger>, ::gateways::IGatewayUserDataStorage& );

    ~UCloudNmaClient();

    inline bool isConnected() const {
        return m_isConnected;
    }

    bool joinToBus() override;

    void updateProvisionDataRequest() override;

    ClbkRet update( UpdContext& ctx ) override;

    void shutdown();

    void getAbstractList();

    inline auto nmaServer() const {
        return m_nmaServer;
    }

    auto nmaPort() const {
        return m_nmaPort;
    }

    inline void setNmaServerSettings( std::string&& nmaServer, const unsigned int nmaPort ) {
        m_nmaServer = std::move( nmaServer );
        m_nmaPort = nmaPort;
    }

    void onRegistered();

protected:
    bool initializeImpl() override;

    // IBusCommandVisitor
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::nma::ServerRequest& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::nma::ClientClbk& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::HandleEventUpdateProductName& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::LocalModeFeatureStatusUpdate& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::CloudDevicesFeatureStatusUpdate& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::AdvancedScenesFeatureStatusUpdate& command ) override;

private:
    void connectToNma( const std::string& srvName, const unsigned int srvPort );

    void onConnected();

    void onConnLost();

    inline bool isValidNmaAddress() {
        return !nmaServer().empty();
    }

    bool tryConnectToNma();
    void provisionFlow();
    void getNewNmaAddress( bool force = false );
    void connectToNmaFlow();
    bool updateProvisionDataIfNeeded();

    bool sendFirmwareUpdateRequest();

    bool sendRegisterHubRequest();

    static network::ISocket::UPtr createSocket();

private:
    std::size_t m_connectionAttempts = 0;
    bool m_isConnected = true;
    bool m_isConnectionSecured = true;
    bool m_provisionUpdated = false;

    std::queue<std::unique_ptr<bus::command::HandleEvent>> m_deferredSend;

    std::chrono::steady_clock::time_point m_nmaUpdateTimepoint;

    bus::EventsHandler m_eventsHandler;

    std::string m_nmaServer;
    unsigned int m_nmaPort = 0;

    std::chrono::steady_clock::time_point m_fwUpdateTimepoint;
};

}  // namespace ucloud_client
