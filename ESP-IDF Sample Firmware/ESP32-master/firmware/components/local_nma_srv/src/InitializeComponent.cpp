#include "ProvisionPeersInfoManager.h"
#include "LocalNmaSrvTask.h"
#include "LocalNmaSrvStartupService.h"

#include <list>

#include "DebugUtils.h"

namespace local_nma_srv {

bool initializeComponent( std::shared_ptr<core::EspLogger> logger,
                          ::gateways::IGatewayUserDataStorage& stor,
                          std::list<core::Thread>& threadList ) {
    std::chrono::seconds provisionManagerShutdownDelay( 1 );
    auto provisionManager = provision::peers::InfoManager::createSingleton( provisionManagerShutdownDelay );
    if( !provisionManager ) {
        FUNC_LOGE( "Can't get InfoManager singleton" );
        return false;
    }
    auto pubCert = provisionManager->getTlsPublicCertificate();
    auto privKey = provisionManager->getTlsPrivateKey();
    core::upd_srv::add( std::move( provisionManager ), "InfoManager" );

    if( pubCert.empty() || privKey.empty() ) {
        constexpr std::chrono::seconds shutdownDelay( 0 );
        auto srv = std::make_unique<LocalNmaSrvStartupService>(
            shutdownDelay, logger, std::ref( stor ), std::ref( threadList ) );
        if( !srv || !srv->initialize() ) {
            FUNC_LOGE( "Can't start LocalNmaSrvStartupService" );
            return false;
        }

        core::upd_srv::add( std::move( srv ), "LocalNmaSrvStartupService" );
    }
    else if( !createNmaServerThread( std::move( pubCert ), std::move( privKey ), logger, stor, threadList ) ) {
        FUNC_LOGE( "Can't create nma server thread" );
        return false;
    }

    return true;
}

}  // namespace local_nma_srv
