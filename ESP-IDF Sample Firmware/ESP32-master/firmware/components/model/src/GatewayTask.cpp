#include <GatewayTask.h>
#include <IGateway.h>
#include <Ezlo.h>
#include <GatewayCommand.h>
#include "Thread.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace gateways {

void threadTask( IGateway* gateway ) {
    if( gateway ) {
        IGateway::UpdContext ctx;
        while( !core::Ezlo::isShuttingDown() && IGateway::ClbkRet::Continue == gateway->update( ctx ) ) {
            ctx.yield();
        }
    }
}

}  // namespace gateways
