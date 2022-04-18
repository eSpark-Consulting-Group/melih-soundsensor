#include "LocalTcpSrvTask.h"
#include "TCPServer2.h"
#include "LocalTcpSrvNmaClient.h"
#include "Ezlo.h"
#include "Thread.h"

#include <list>

#include "DebugUtils.h"

namespace local_tcp_srv {

void threadTask( int port,
                 int maxConn,
                 std::shared_ptr<core::EspLogger> espLogger,
                 ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage ) {
    network::TCPServer2 srv( maxConn );
    std::list<LocalTcpSrvNmaClient::UPtr> clients;

    core::upd_srv::UpdContext ctx;
    while( true ) {
        if( core::Ezlo::isShuttingDown() ) {
            break;
        }

        if( !srv.isListening() ) {
            if( !srv.listen( std::string(), port ) ) {
                srv.close();
                core::this_thread::sleep_for( std::chrono::seconds( 1 ) );
                continue;
            }
        }

        {
            auto sock = srv.nextPendingConnection();
            bool isSuccess = sock && sock->setWriteTimeout( std::chrono::seconds( 5 ) ) &&
                             sock->setReadTimeout( std::chrono::milliseconds( 100 ) );
            if( isSuccess ) {
                auto client =
                    std::make_unique<LocalTcpSrvNmaClient>( std::move( sock ), espLogger, gatewayUserDataStorage );
                if( client->initialize() && client->joinToBus() ) {
                    clients.emplace_back( std::move( client ) );
                }
            }
        }

        {
            const auto end = std::end( clients );
            auto itr = std::begin( clients );
            while( end != itr ) {
                if( LocalTcpSrvNmaClient::ClbkRet::Continue == ( *itr )->update( ctx ) ) {
                    itr = std::next( itr );
                }
                else {
                    itr = clients.erase( itr );
                }
            }
        }

        ctx.yield();
    }
}

}  // namespace local_tcp_srv