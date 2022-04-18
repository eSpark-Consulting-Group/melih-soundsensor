#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "services/WifiBootstrapTask.h"
#include "cloud_nma_client/Utils.h"
#include "Thread.h"
#include "wifi_service2/Common.h"
#include "hub_core/Common.h"
#include "Storage.h"
#include "StorageFile.h"

#ifdef CONFIG_MBEDTLS_HAVE_TIME
#include "time_service/Utils.h"
#endif // CONFIG_MBEDTLS_HAVE_TIME

#ifdef CONFIG_ENABLE_LOCAL_TCP_SRV
#include "LocalTcpSrvTask.h"
#endif // CONFIG_ENABLE_LOCAL_TCP_SRV

#include "DebugUtils.h"

namespace sys_serv {

namespace {  // Should be removed in the future

struct OldWifiConfig {
    char ssid[33];
    uint8_t bssid[6];
    char password[64];
    uint8_t channel;
};

constexpr char wifiConfigFilePath[] = "/vol/wificonfig.dat";
constexpr uint8_t StorageMajorVersion = 1;
constexpr uint8_t StorageMinorVersion = 0;

std::optional<OldWifiConfig> getOldSavedConfig() {
    storage::File file;
    if( !file.open( wifiConfigFilePath, "rb" ) ) {
        return std::nullopt;
    }

    uint8_t version;
    bool isSuccess = file.read( version );
    isSuccess = isSuccess && ( StorageMajorVersion == version );
    isSuccess = isSuccess && file.read( version );
    isSuccess = isSuccess && ( ( StorageMinorVersion < version ) || ( StorageMinorVersion == version ) );
    union {
        OldWifiConfig config;
        char buff[sizeof( OldWifiConfig ) + 1];
    };
    isSuccess = isSuccess && file.readString( buff, sizeof( buff ) );
    return isSuccess ? std::make_optional( config ) : std::nullopt;
}

}  // namespace

void wifiBootstrapTask( std::list<core::Thread>             &threads,
                        std::shared_ptr<core::EspLogger>     espLogger,
                        ::gateways::IGatewayUserDataStorage &gatewayUserDataStorage,
                        const std::atomic<bool>             &isRunMainCycle ) {
    while( !isRunMainCycle ) {
        core::this_thread::sleep_for( std::chrono::milliseconds( 500 ));
    }
    
    {   
        std::chrono::seconds wifiServiceShutdownDelay( 4 );
        if( !network::wifi_srv2::initWifiService( wifiServiceShutdownDelay ) ) {
            FUNC_LOGE("Can't initialize wifi service" );
            return;
        }
        FUNC_LOGI("Successfully initialize wifi service"); 

        auto oldConfig = getOldSavedConfig();
        if ( oldConfig ) {
            FUNC_LOGW( "Use wifi point creds from old config: ssid=%s, passw=%s", oldConfig->ssid, oldConfig->password );
        }
        if( oldConfig && network::wifi_srv2::joinToAccessPoint( oldConfig->ssid, oldConfig->password ) ) {
            storage::Storage::get().remove( wifiConfigFilePath );
        }
        else if (network::wifi_srv2::hasValidStaConfig() ) {
            network::wifi_srv2::joinToLastAccessPoint();
        }
        else {
            network::wifi_srv2::startAccessPoint( core::hubName() );
        }

        while( !network::wifi_srv2::isConnectedToAcessPoint() ) {
            core::this_thread::sleep_for( std::chrono::seconds( 1 ));
        }
    }

#ifdef CONFIG_MBEDTLS_HAVE_TIME
    time_service::waitingTimeDidEstablished();
#endif // CONFIG_MBEDTLS_HAVE_TIME

#ifdef CONFIG_ENABLE_LOCAL_TCP_SRV
    {
        constexpr const auto tcpLocalNmaServerPort = 7676;
        constexpr const auto tcpLocalNmaServerMaxConn = 10;
        core::Thread tcpLocalNmaServerThread( "LNMA_SRV_TASK",
                                              3 * 1024,
                                              local_tcp_srv::threadTask,
                                              tcpLocalNmaServerPort,
                                              tcpLocalNmaServerMaxConn,
                                              espLogger,
                                              std::ref( gatewayUserDataStorage ) );
        if( tcpLocalNmaServerThread ) {
            threads.push_back( std::move( tcpLocalNmaServerThread ) );
        }
        else {
            FUNC_LOGI( "Can't run local_tcp_srv::threadTask!" );
        }
    }
#endif  // CONFIG_ENABLE_LOCAL_TCP_SRV

    cloud_client::createAndRunStartupService( std::chrono::seconds( 0 ), espLogger, gatewayUserDataStorage );
}

}  // namespace sys_serv

#endif  // not CONFIG_UART_API
