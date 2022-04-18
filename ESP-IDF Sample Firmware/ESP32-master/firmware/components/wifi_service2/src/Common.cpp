#include "wifi_service2/Common.h"
#include "WifiGeneric.h"
#include "wifi_service2/WifiCommands.h"
#include "PrivateWifiCommands.h"
#include "UpdatableService.h"
#include "Ezlo.h"
#include "syscore/commands/Shutdown.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
#include "WifiSniffer.h"
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

extern "C" {
#include <sys/socket.h>
#include <esp_log.h>
#include <lwip/dns.h>
}  // extern "C"

#include <cstdio>

#include "DebugUtils.h"

#ifndef EXAMPLE_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#endif  // EXAMPLE_ESP_WIFI_SSID

#ifndef EXAMPLE_ESP_WIFI_PASS
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#endif  // EXAMPLE_ESP_WIFI_PASS

namespace network {
namespace wifi_srv2 {

std::atomic_bool g_isConnectedToAccessPoint{ false };

bool initWifiService( const std::chrono::milliseconds& shutdownDelay ) {
    auto wifiGen = WiFiGenericClass::createSingleton( shutdownDelay );
    if( !wifiGen ) {
        return false;
    }

    core::upd_srv::add( std::move( wifiGen ), "WifiGen" );
    return true;
}

bool joinToLastAccessPoint() {
    return bus::sendFromNowhere<command::JoinToLastAccessPoint>( command::BUS_NAME );
}

bool joinToAccessPoint( const std::string& _ssid, const std::string& _passw, const std::optional<BssidType>& _bssid ) {
    constexpr auto channel = 0;
    return bus::sendFromNowhere<command::JoinToAccessPoint>( command::BUS_NAME, _ssid, _passw, channel, _bssid );
}

bool disjoinFromAccessPoint( const std::string& apName, bool _isEraseAp ) {
    return bus::sendFromNowhere<command::DisjoinFromAccessPoint>( command::BUS_NAME, apName, _isEraseAp );
}

const std::atomic_bool& isConnectedToAcessPoint() {
    return g_isConnectedToAccessPoint;
}

std::optional<IpInfo> getStaIpInfo() {
    auto wlan0 = WiFiGenericClass::get();
    return wlan0 ? wlan0->getStaIpInfo() : std::nullopt;
}

IPAddress dnsIP( const unsigned int dns_no ) {
    const ip_addr_t* dns_ip = dns_getserver( dns_no );
    return dns_ip ? IPAddress( dns_ip->u_addr.ip4.addr ) : IPAddress();
}

std::optional<StaConfig> getStaConfig() {
    wifi_config_t conf;
    return ESP_OK == esp_wifi_get_config( WIFI_IF_STA, &conf ) ? std::make_optional( conf.sta ) : std::nullopt;
}

bool setStaConfig( const StaConfig& config ) {
    wifi_config_t conf;
    conf.sta = config;
    return ESP_OK == esp_wifi_set_config( WIFI_IF_STA, &conf );
}

bool hasValidStaConfig() {
    const auto conf = getStaConfig();
    return conf && conf->isValid();
}

bool wipeStaConfig() {
    wifi_config_t conf{};
    return ESP_OK == esp_wifi_set_config( WIFI_IF_STA, &conf );
}

std::optional<WifiApInfo> getStaApInfo() {
    WifiApInfo info;
    return ESP_OK == esp_wifi_sta_get_ap_info( &info ) ? std::make_optional( info ) : std::nullopt;
}

std::string ssidStr() {
    auto info = getStaApInfo();
    return info ? info->ssidToString() : std::string();
}

std::optional<BssidType> bssid() {
    auto info = getStaApInfo();
    return info ? std::make_optional<BssidType>( info->toArray() ) : std::nullopt;
}

std::optional<uint8_t> rssi() {
    auto info = getStaApInfo();
    return info ? std::make_optional( info->rssi ) : std::nullopt;
}

std::string bssidStr() {
    auto id = bssid();
    if( !id ) {
        return {};
    }

    char mac[18];
    sprintf(
        mac, "%02X:%02X:%02X:%02X:%02X:%02X", ( *id )[0], ( *id )[1], ( *id )[2], ( *id )[3], ( *id )[4], ( *id )[5] );
    return mac;
}

//-----------------------------------------------------------------------------------

bool startAccessPoint( const std::string& ssid,
                       const std::string& passphrase,
                       int channel,
                       bool isSsidHidden,
                       unsigned int maxConnections ) {
    return bus::sendFromNowhere<command::StartAccessPoint>(
        command::BUS_NAME, ssid, passphrase, channel, isSsidHidden, maxConnections );
}

bool stopAccessPoint() {
    return bus::sendFromNowhere<command::StopAccessPoint>( command::BUS_NAME );
}

//-----------------------------------------------------------------------------------

bool startWifiScan() {
    return bus::sendFromNowhere<command::StartWifiScan>( command::BUS_NAME );
}

bool stopWifiScan() {
    return bus::sendFromNowhere<command::StopWifiScan>( command::BUS_NAME );
}

std::future<WifiApInfoSetType> getAvailableAccessPointsAsync() {
    class ResultWaiter : public core::Shutdownable {
    public:
        using ClockType = std::chrono::steady_clock;
        using TimepointType = ClockType::time_point;
        using ClbkRet = core::upd_srv::ClbkRet;
        using UpdContext = core::upd_srv::UpdContext;

        ResultWaiter( std::chrono::seconds timeout = std::chrono::seconds( 20 ) )
        : m_timeout( timeout ) {}

        ~ResultWaiter() {
            if( !m_isValueSetted ) {
                m_promise.set_value( WifiApInfoSetType{} );
            }
        }

        auto getFuture() {
            return m_promise.get_future();
        }

    private:
        bool initializeImpl() override {
            if( !core::Shutdownable::initializeImpl() ) {
                return false;
            }

            m_timepoint = ClockType::now();
            return bus::Bus::get().join( responce::BUS_NAME, *this ) && startWifiScan();
        }

        void visit( const bus::BusAddr from,
                    const bus::BusAddr to,
                    const responce::DiscoveredWifiAccessPoints& cmd ) override {
            m_promise.set_value( cmd.apInfo() );
            m_isValueSetted = true;
        }

        ClbkRet updateComponent( UpdContext& ctx ) override {
            return !m_isValueSetted && m_timeout > ClockType::now() - m_timepoint ? ClbkRet::Continue :
                                                                                    ClbkRet::Completed;
        };

    private:
        std::promise<WifiApInfoSetType> m_promise;
        std::chrono::seconds m_timeout;
        TimepointType m_timepoint;
        bool m_isValueSetted{ false };
    };

    auto instance = std::make_unique<ResultWaiter>();
    if( !instance || !instance->initialize() ) {
        return std::future<WifiApInfoSetType>{};
    }
    auto future = instance->getFuture();

    auto uuid = core::upd_srv::add( std::move( instance ) );
    return uuid ? std::move( future ) : std::future<WifiApInfoSetType>{};
}

bool wipeWifiSettings() {
    return ESP_OK == esp_wifi_restore();
}

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
bool startWifiSniffer() {
    auto wifiSniffer = WifiSniffer::createSingleton( SNIFFER_INTF_WLAN );
    if( !wifiSniffer ) {
        FUNC_LOGE( "Can't create WifiSniffer singleton" );
        return false;
    }

    const auto uuid = core::upd_srv::add( std::move( wifiSniffer ), "WifiSniffer" );
    return true;
}

bool stopWifiSniffer() {
    return bus::sendFromNowhere<core::commands::Shutdown>( WifiSniffer::BUS_NAME );
}
#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

}  // namespace wifi_srv2
}  // namespace network
