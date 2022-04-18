#include "LocalTcpSrvNmaClient.h"
#include "LocalTcpNmaHandler.h"

#include "DebugUtils.h"

namespace local_tcp_srv {

LocalTcpSrvNmaClient::LocalTcpSrvNmaClient( network::ISocket::UPtr&& sock,
                                            std::shared_ptr<core::EspLogger> espLogger,
                                            ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage )
: NmaBase( std::move( sock ), espLogger, gatewayUserDataStorage ) {
    setJsonHandler( std::make_unique<LocalTcpNmaHandler>( *this, espLogger, gatewayUserDataStorage ) );
}

LocalTcpSrvNmaClient::~LocalTcpSrvNmaClient() {
    FUNC_LOGI( "Destroy LocalTcpSrvNmaClient instance" );
}

}  // namespace local_tcp_srv