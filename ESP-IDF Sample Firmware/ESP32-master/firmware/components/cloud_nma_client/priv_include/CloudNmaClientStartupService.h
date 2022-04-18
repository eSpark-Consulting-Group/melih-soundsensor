#pragma once

#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "Shutdownable.h"

#include <memory>

namespace gateways {
class IGatewayUserDataStorage;
}
namespace core {
class EspLogger;
}  // namespace core

namespace cloud_client {

class CloudNmaClientStartupService : public core::Shutdownable {
public:
    using ParentType = core::Shutdownable;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    static bool createAndRun( const std::chrono::seconds& delay,
                              std::shared_ptr<core::EspLogger>,
                              ::gateways::IGatewayUserDataStorage& );

private:
    CloudNmaClientStartupService( const std::chrono::seconds&,
                                  std::shared_ptr<core::EspLogger>,
                                  ::gateways::IGatewayUserDataStorage& );
    ClbkRet updateComponent( UpdContext& ctx ) override;

    bool initializeImpl() override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                const cloud_client::commands::HubProvisionChanged& command ) override;

private:
    std::weak_ptr<core::EspLogger> m_espLogger;
    ::gateways::IGatewayUserDataStorage& m_gatewayUserDataStorage;
};

bool restartStartupService();

}  // namespace cloud_client

#endif  // not CONFIG_UART_API
