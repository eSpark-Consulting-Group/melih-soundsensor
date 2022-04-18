
#include "LocalUartSrvTask.h"

#include "LocalUartSrv.h"

#include <Ezlo.h>

#include "Thread.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <DebugUtils.h>

namespace ucloud_client {

void threadTask( std::shared_ptr<core::EspLogger> espLogger,
                 ::gateways::IGatewayUserDataStorage& gatewayUserDataStorage ) {
    std::unique_ptr<UCloudNmaClient> client;

    while( true ) {
        if( core::Ezlo::isShuttingDown() ) {
            goto exit;
        }

        client = std::make_unique<UCloudNmaClient>( espLogger, gatewayUserDataStorage );
        if( client->initialize() ) {
            break;
        }

        FUNC_LOGE( "Can't initialize nma::CloudNmaClient!" );
        client->shutdown();
        client.reset();
        vTaskDelay( 500 / portTICK_PERIOD_MS );
    }

    {
        UCloudNmaClient::UpdContext ctx;
        while( true ) {
            vTaskDelay( 10 / portTICK_PERIOD_MS );
            if( core::Ezlo::isShuttingDown() ) {
                client->shutdown();
                goto exit;
            }

            client->update( ctx );
            ctx.yield();
        }  // end while(true)
    }

exit:
    vTaskDelay( 50 / portTICK_PERIOD_MS );
}

}  // namespace ucloud_client
