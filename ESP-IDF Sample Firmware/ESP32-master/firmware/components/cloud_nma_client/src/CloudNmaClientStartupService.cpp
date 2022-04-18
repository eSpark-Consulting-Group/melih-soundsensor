#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "CloudNmaClientStartupService.h"
#include "CloudNmaClient.h"

#ifdef CONFIG_SPIRAM
#include "Thread.h"
#endif  // CONFIG_SPIRAM

#include "cloud_client/commands/HubProvisionChanged.h"
#include "wifi_service2/Common.h"

#include "DebugUtils.h"

namespace cloud_client {

using namespace std::chrono_literals;

namespace {
std::chrono::seconds g_delay;
std::weak_ptr<core::EspLogger> g_espLogger;
::gateways::IGatewayUserDataStorage* g_gatewayUserDataStorage{ nullptr };
}  // namespace

bool restartStartupService() {
    if( !g_gatewayUserDataStorage ) {
        return false;
    }

    auto logger = g_espLogger.lock();
    return logger && CloudNmaClientStartupService::createAndRun( g_delay, logger, *g_gatewayUserDataStorage );
}

bool CloudNmaClientStartupService::createAndRun( const std::chrono::seconds& delay,
                                                 std::shared_ptr<core::EspLogger> espLogger,
                                                 ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage ) {
    constexpr std::chrono::seconds shutdownDelay( 0 );
    auto srv = std::unique_ptr<CloudNmaClientStartupService>(
        new CloudNmaClientStartupService( shutdownDelay, espLogger, gatewayUserDataStorage ) );
    if( !srv->initialize() ) {
        return false;
    }

    if( 0 < delay.count() ) {
        srv->setDelay( delay );
    }

    core::upd_srv::add( std::move( srv ), "CloudNmaClientStartupService" );

    g_delay = delay;
    g_espLogger = espLogger;
    g_gatewayUserDataStorage = &gatewayUserDataStorage;

    return true;
}

bool CloudNmaClientStartupService::initializeImpl() {
    return ParentType::initializeImpl() && bus::Bus::get().join( cloud_client::commands::BUS_NAME, *this );
}

CloudNmaClientStartupService::CloudNmaClientStartupService(
    const std::chrono::seconds& delay,
    std::shared_ptr<core::EspLogger> espLogger,
    ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage )
: ParentType( delay )
, m_espLogger( espLogger )
, m_gatewayUserDataStorage( gatewayUserDataStorage ) {}

CloudNmaClientStartupService::ClbkRet CloudNmaClientStartupService::updateComponent( UpdContext& ctx ) {
    if( !network::wifi_srv2::isConnectedToAcessPoint() ) {
        return setDelay( std::chrono::seconds( 1 ) );
    }

    auto logger = m_espLogger.lock();
    if( !logger ) {
        return CloudNmaClientStartupService::ClbkRet::Completed;
    }

    auto client = std::make_unique<CloudNmaClient>( logger, m_gatewayUserDataStorage );
    if( !client || !client->initialize() ) {
        return setDelay( 10s );
    }

#ifdef CONFIG_SPIRAM
    auto threadTask = []( CloudNmaClient::UPtr& client ) {
        client->setRestartStartupService();
        if( !client->performConnect() ) {
            return;
        }

        core::upd_srv::UpdContext ctx;
        while( CloudNmaClient::ClbkRet::Completed != client->update( ctx ) ) {
            ctx.yield();
        }
    };
    core::Thread thread( "CloudNmaClient", 8 * 1024, threadTask, std::move( client ) );
    if( thread ) {
        thread.detach();
    }
    else {
        return setDelay( 10s );
    }
#else   // not CONFIG_SPIRAM
    if( !client->performConnect() ) {
        return setDelay( 10s );
    }
    client->setRestartStartupService();
    core::upd_srv::add( std::move( client ), "CloudNmaClient" );
#endif  // CONFIG_SPIRAM
    return ClbkRet::Completed;
}

void CloudNmaClientStartupService::visit( const bus::BusAddr from,
                                          const bus::BusAddr to,
                                          const cloud_client::commands::HubProvisionChanged& command ) {
    cloud_client::CloudNmaClient::setServerInfo(
        command.nmaSrvName(), command.nmaSrvPort(), command.isConnectionSecured() );
}

}  // namespace cloud_client

#endif  // not CONFIG_UART_API
