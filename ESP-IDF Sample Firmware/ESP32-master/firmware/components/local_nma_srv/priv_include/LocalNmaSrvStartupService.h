#pragma once

#include "Shutdownable.h"

#include <memory>
#include <vector>
#include <list>

namespace gateways {
class IGatewayUserDataStorage;
}

namespace core {
class EspLogger;
class Thread;
}  // namespace core

namespace local_nma_srv {

class LocalNmaSrvStartupService : public core::Shutdownable {
public:
    using ParentType = core::Shutdownable;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    LocalNmaSrvStartupService( const std::chrono::seconds&,
                               std::shared_ptr<core::EspLogger>,
                               ::gateways::IGatewayUserDataStorage&,
                               std::list<core::Thread>& );

private:
    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                cloud_client::responces::ReceivedPeersProvision& ) override;

private:
    std::shared_ptr<core::EspLogger> m_espLogger;
    ::gateways::IGatewayUserDataStorage& m_gatewayUserDataStorage;
    std::list<core::Thread>& m_threads;
    bool m_isProvisionReceived = false;
    std::vector<char> m_pubCert;
    std::vector<char> m_privKey;
};

bool createNmaServerThread( std::vector<char>,
                            std::vector<char>,
                            std::shared_ptr<core::EspLogger>,
                            ::gateways::IGatewayUserDataStorage&,
                            std::list<core::Thread>& );

}  // namespace local_nma_srv
