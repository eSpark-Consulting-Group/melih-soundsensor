#pragma once

#include <memory>

namespace gateways {
class IGatewayUserDataStorage;
}
namespace core {
class EspLogger;
}  // namespace core

namespace local_tcp_srv {

void threadTask( int port, int maxConn, std::shared_ptr<core::EspLogger>, ::gateways::IGatewayUserDataStorage& );

}  // namespace local_tcp_srv