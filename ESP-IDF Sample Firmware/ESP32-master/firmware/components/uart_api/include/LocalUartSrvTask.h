
#pragma once

#include <memory>

namespace gateways {
class IGatewayUserDataStorage;
}
namespace core {
class Ezlo;
class EspLogger;
}  // namespace core

namespace network {
class WiFiManager;
}  // namespace network

namespace ucloud_client {

void threadTask( std::shared_ptr<core::EspLogger>, ::gateways::IGatewayUserDataStorage& );

}  // namespace ucloud_client
