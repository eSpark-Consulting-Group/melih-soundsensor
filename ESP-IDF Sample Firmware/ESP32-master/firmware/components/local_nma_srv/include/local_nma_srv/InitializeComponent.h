#pragma once

#include "Thread.h"

#include <list>
#include <memory>

namespace core {
class EspLogger;
}  // namespace core

namespace gateways {
class IGatewayUserDataStorage;
}  // namespace gateways

namespace local_nma_srv {

bool initializeComponent( std::shared_ptr<core::EspLogger> logger,
                          ::gateways::IGatewayUserDataStorage& stor,
                          std::list<core::Thread>& threadList );

}  // namespace local_nma_srv
