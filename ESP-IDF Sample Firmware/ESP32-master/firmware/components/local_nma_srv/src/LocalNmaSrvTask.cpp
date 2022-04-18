#include "LocalNmaSrvTask.h"
#include "wifi_service2/Common.h"
#include "Ezlo.h"
#include "MDnsServer.h"
#include "Thread.h"

#include "DebugUtils.h"

namespace local_nma_srv {

using namespace std::chrono_literals;

void threadTask( std::vector<char> pubCert,
                 std::vector<char> privKey,
                 std::shared_ptr<core::EspLogger> espLogger,
                 ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage ) {
    auto server = std::make_shared<network::WSServer>( std::move( pubCert ), std::move( privKey ) );
    std::list<std::unique_ptr<LocalNmaClient>> clients;

    core::upd_srv::UpdContext ctx;
    while( true ) {
        {
            if( core::Ezlo::isShuttingDown() ) {
                goto exit;
            }
        }

        if( !server->isListening() ) {
            if( !clients.empty() ) {
                clients.clear();
            }

            while( !network::wifi_srv2::isConnectedToAcessPoint() ) {
                ctx.sleep_for( 5s );
            }

            {
                if( !server->listen( nullptr, core::Ezlo::getOfflineServerPort() ) ) {
                    FUNC_LOGE( "Initialization of WSServer failed!" );
                    goto exit;
                }
            }

            network::mdns::start();
            ctx.tryYield();
            network::mdns::setUpService();
            FUNC_LOGI( "MDNS started" );
            ctx.tryYield();
        }

        {
            auto socket = server->nextPendingConnection();
            if( socket ) {
                auto client =
                    std::make_unique<LocalNmaClient>( std::move( socket ), espLogger, gatewayUserDataStorage );
                if( client && client->initialize() ) {
                    clients.emplace_back( std::move( client ) );
                }
            }
        }

        {
            const auto end = std::end( clients );
            auto itr = std::begin( clients );
            while( end != itr ) {
                ctx.tryYield();
                if( LocalNmaClient::ClbkRet::Continue == ( *itr )->update( ctx ) ) {
                    itr = std::next( itr );
                }
                else {
                    itr = clients.erase( itr );
                }
            }
        }

        ctx.yield();
    }

exit:
    if( !clients.empty() ) {
        clients.clear();
        FUNC_LOGI( "Offline server clients closed" );
    }

    if( server->isListening() ) {
        if( server->close() ) {
            FUNC_LOGI( "Offline server closed successfully" );
        }
        else {
            FUNC_LOGE( "Failed to close offline server!" );
        }
    }
}

}  // namespace local_nma_srv