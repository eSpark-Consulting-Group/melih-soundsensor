#include "LocalNmaSrvStartupService.h"

#include "Thread.h"
#include "LocalNmaSrvTask.h"
#include "cloud_client/responces/ReceivedPeersProvision.h"

#include "DebugUtils.h"

namespace local_nma_srv {

LocalNmaSrvStartupService::LocalNmaSrvStartupService( const std::chrono::seconds& delay,
                                                      std::shared_ptr<core::EspLogger> espLogger,
                                                      ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage,
                                                      std::list<core::Thread>& threads )
: ParentType( delay )
, m_espLogger( espLogger )
, m_gatewayUserDataStorage( gatewayUserDataStorage )
, m_threads( threads ) {}

bool LocalNmaSrvStartupService::initializeImpl() {
    return ParentType::initializeImpl() && bus::Bus::get().join( cloud_client::responces::BUS_NAME, *this );
}

LocalNmaSrvStartupService::ClbkRet LocalNmaSrvStartupService::updateComponent( UpdContext& ctx ) {
    if( !m_isProvisionReceived ) {
        return setDelay( std::chrono::seconds( 1 ) );
    }

    if( m_pubCert.empty() || m_privKey.empty() ) {
        FUNC_LOGE( "Public certificate or private key is empty!" );
        return ClbkRet::Completed;
    }

    createNmaServerThread(
        std::move( m_pubCert ), std::move( m_privKey ), m_espLogger, m_gatewayUserDataStorage, m_threads );

    return ClbkRet::Completed;
}

void LocalNmaSrvStartupService::visit( const bus::BusAddr from,
                                       const bus::BusAddr to,
                                       cloud_client::responces::ReceivedPeersProvision& resp ) {
    if( m_isProvisionReceived ) {
        return;
    }

    m_pubCert = std::move( resp.pubCert() );
    m_privKey = std::move( resp.privKey() );
    m_isProvisionReceived = true;
}

bool createNmaServerThread( std::vector<char> pubCert,
                            std::vector<char> privKey,
                            std::shared_ptr<core::EspLogger> espLogger,
                            ::gateways::IGatewayUserDataStorage& gatewayUserDataInMemoryStorage,
                            std::list<core::Thread>& threads ) {
    core::Thread nmaServerThread( "NMA_SRV",
                                  8 * 1024,
                                  local_nma_srv::threadTask,
                                  std::move( pubCert ),
                                  std::move( privKey ),
                                  espLogger,
                                  std::ref( gatewayUserDataInMemoryStorage ) );
    if( !nmaServerThread ) {
        FUNC_LOGE( "Can't run local_nma_srv::threadTask!" );
        return false;
    }

    threads.push_back( std::move( nmaServerThread ) );
    return true;
}

}  // namespace local_nma_srv
