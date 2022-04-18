#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "Shutdownable.h"
#include "wifi_service2/WifiCommands.h"

namespace network {
namespace wifi_srv2 {

class sniffer_runtime_t;

enum sniffer_intf_t {
    SNIFFER_INTF_WLAN = 0, /*!< WLAN interface */
};

class WifiSniffer : public core::SingletonBaseImpl<WifiSniffer> {
public:
    using ContextUPtr = std::unique_ptr<sniffer_runtime_t>;
    using ParentType = core::SingletonBaseImpl<WifiSniffer>;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    using SPtr = std::shared_ptr<WifiSniffer>;

    static const char* BUS_NAME;

    WifiSniffer( const sniffer_intf_t intf );

    ~WifiSniffer();

    bool start();

    bool stop();

private:
    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override;

private:
    ContextUPtr m_ctx;
    ClockType::time_point m_sendTp, m_updateTp;
    responce::DiscoveredWifiBeacons::AddrToBeaconInfo m_beacons;
    bool m_isStarted{ false };
    bool m_isShouldStarted{ false };
};

}  // namespace wifi_srv2
}  // namespace network

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
