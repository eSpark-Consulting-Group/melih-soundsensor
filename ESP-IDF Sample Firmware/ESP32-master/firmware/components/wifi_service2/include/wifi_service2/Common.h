#pragma once

#include "sdkconfig.h"

#include "WifiTypes.h"
#include "IPAddress.h"

#include <chrono>
#include <future>

namespace network {
namespace wifi_srv2 {

bool initWifiService( const std::chrono::milliseconds& shutdownDelay );

bool joinToLastAccessPoint();

bool joinToAccessPoint( const std::string& _ssid,
                        const std::string& _passw,
                        const std::optional<BssidType>& bssid = std::nullopt );

bool disjoinFromAccessPoint( const std::string& apName = std::string{}, bool _isEraseAp = false );

const std::atomic_bool& isConnectedToAcessPoint();

// STA network info
std::optional<IpInfo> getStaIpInfo();
IPAddress dnsIP( const unsigned int dns_no = 0 );

std::optional<StaConfig> getStaConfig();
bool setStaConfig( const StaConfig& config );
bool hasValidStaConfig();
bool wipeStaConfig();

std::optional<WifiApInfo> getStaApInfo();
std::string ssidStr();
std::optional<BssidType> bssid();
std::string bssidStr();
std::optional<uint8_t> rssi();

//--------------------------------------------------------------------

bool startAccessPoint( const std::string& ssid,
                       const std::string& passphrase = std::string(),
                       int channel = 1,
                       bool isSsidHidden = false,
                       unsigned int maxConnections = 4 );

bool softAPConfig( IPAddress local_ip, IPAddress gateway, IPAddress subnet );

bool stopAccessPoint();

//--------------------------------------------------------------------

bool startWifiScan();

bool stopWifiScan();

std::future<WifiApInfoSetType> getAvailableAccessPointsAsync();

//--------------------------------------------------------------------

bool wipeWifiSettings();

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
bool startWifiSniffer();

bool stopWifiSniffer();
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

}  // namespace wifi_srv2
}  // namespace network
