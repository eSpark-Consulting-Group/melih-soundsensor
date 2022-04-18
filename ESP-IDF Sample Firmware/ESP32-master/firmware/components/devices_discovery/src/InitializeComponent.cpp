#include "dev_dis/InitializeComponent.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "dev_dis/DevDiscovery.h"

#include "DebugUtils.h"

namespace dev_dis {

using namespace std::chrono_literals;

bool initializeComponent() {
    constexpr auto shutdownDelay = 0s;
    auto instance = DevDiscovery::createSingleton( shutdownDelay );
    if( !instance ) {
        FUNC_LOGI( "Can't create DevDiscovery service" );
        return false;
    }

    core::upd_srv::add( instance, "DevDiscovery" );

    return true;
}

}  // namespace dev_dis

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
