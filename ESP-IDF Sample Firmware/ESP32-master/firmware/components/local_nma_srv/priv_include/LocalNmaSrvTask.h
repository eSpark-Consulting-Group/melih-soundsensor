#pragma once

#include "WebSocketServer.h"
#include "LocalNmaSrvClient.h"

#include <memory>
#include <list>

namespace gateways {
class IGatewayUserDataStorage;
}
namespace core {
class EspLogger;
}  // namespace core

namespace network {
class WSServer;
}  // namespace network

namespace local_nma_srv {

void threadTask( std::vector<char> pubCert,
                 std::vector<char> privKey,
                 std::shared_ptr<core::EspLogger>,
                 ::gateways::IGatewayUserDataStorage& );

}  // namespace local_nma_srv