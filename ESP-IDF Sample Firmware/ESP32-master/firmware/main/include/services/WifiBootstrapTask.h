
#pragma once

#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include <memory>
#include <atomic>
#include <list>

namespace gateways {
class IGatewayUserDataStorage;
}
namespace core {
class Ezlo;
class EspLogger;
class Thread;
}  // namespace core


namespace sys_serv {

void wifiBootstrapTask( std::list<core::Thread>             &threads,
                        std::shared_ptr<core::EspLogger>     espLogger, 
                        ::gateways::IGatewayUserDataStorage &gatewayUserDataStorage,
                        const std::atomic<bool>             &isRunMainCycle );

}  // namespace sys_serv

#endif  // not CONFIG_UART_API
