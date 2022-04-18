#pragma once

#include "Thread.h"
#include "wifi_service2/WifiTypes.h"

#include <atomic>

namespace network {
namespace wifi_srv2 {

core::Thread runWifiHttpServerThread( std::atomic<HttpServerState>& httpServerState );

}  // namespace wifi_srv2
}  // namespace network