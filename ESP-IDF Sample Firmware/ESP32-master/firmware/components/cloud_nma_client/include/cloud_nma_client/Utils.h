#pragma once

#include "sdkconfig.h"

#include <tuple>
#include <chrono>
#include <memory>

namespace core {
class EspLogger;
}  // namespace core

namespace gateways {
class IGatewayUserDataStorage;
}  // namespace gateways
namespace cloud_client {

using NmaInfo = std::tuple<std::string, unsigned int, bool>;

bool isConnectionExist( const std::string& oldSrvName, const unsigned int oldSrvPort );

// Return nma address, nma port, is connection secured
NmaInfo requestNewNmaAddress( const std::string& oldNmaAddress,
                              const unsigned int oldPort,
                              const unsigned int requestAttempts );

NmaInfo getLatestNma();

#ifndef CONFIG_UART_API
bool createAndRunStartupService( const std::chrono::seconds& delay,
                                 std::shared_ptr<core::EspLogger>,
                                 ::gateways::IGatewayUserDataStorage& );
#endif  // not CONFIG_UART_API
}  // namespace cloud_client
