#pragma once

#include "wifi_service2/IPAddress.h"
#include "wifi_service2/WifiTypes.h"
#include "system/EventGroup.h"
#include "Shutdownable.h"

extern "C" {
#include <esp_err.h>
#include <esp_event.h>
#include <wifi_provisioning/manager.h>
}  // extern "C"

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <chrono>

namespace network {
namespace wifi_srv2 {

struct wifi_prov_event_t {
    wifi_prov_cb_event_t event;
    void* event_data;
};

struct system_prov_event_t {
    wifi_prov_event_t* prov_event;
    system_event_t* sys_event;
};

using wifi_event_id_t = size_t;

enum wifi_power_t {
    WIFI_POWER_19_5dBm = 78,    // 19.5dBm
    WIFI_POWER_19dBm = 76,      // 19dBm
    WIFI_POWER_18_5dBm = 74,    // 18.5dBm
    WIFI_POWER_17dBm = 68,      // 17dBm
    WIFI_POWER_15dBm = 60,      // 15dBm
    WIFI_POWER_13dBm = 52,      // 13dBm
    WIFI_POWER_11dBm = 44,      // 11dBm
    WIFI_POWER_8_5dBm = 34,     // 8.5dBm
    WIFI_POWER_7dBm = 28,       // 7dBm
    WIFI_POWER_5dBm = 20,       // 5dBm
    WIFI_POWER_2dBm = 8,        // 2dBm
    WIFI_POWER_MINUS_1dBm = -4  // -1dBm
};

static const unsigned int AP_STARTED_BIT = BIT0;
static const unsigned int AP_HAS_IP6_BIT = BIT1;

static const unsigned int STA_STARTED_BIT = BIT3;
static const unsigned int STA_CONNECTING_BIT = BIT4;
static const unsigned int STA_CONNECTED_BIT = BIT5;
static const unsigned int STA_HAS_IP4_BIT = BIT6;
static const unsigned int STA_HAS_IP6_BIT = BIT7;

static const unsigned int ETH_STARTED_BIT = BIT8;
static const unsigned int ETH_CONNECTED_BIT = BIT9;
static const unsigned int ETH_HAS_IP_BIT = BIT10;
static const unsigned int ETH_HAS_IP6_BIT = BIT11;

static const unsigned int WIFI_SCANNING_BIT = BIT12;

static const unsigned int WIFI_DNS_IDLE_BIT = BIT13;
static const unsigned int WIFI_DNS_DONE_BIT = BIT14;

static const unsigned int ESP_WIFI_STARTED_BIT = BIT15;

class WiFiGenericClass : public core::SingletonBaseImpl<WiFiGenericClass> {
    friend class core::SingletonBaseImpl<WiFiGenericClass>;

public:
    using ParentType = core::SingletonBaseImpl<WiFiGenericClass>;
    using ClbkRet = ParentType::ClbkRet;

    ~WiFiGenericClass();

    std::optional<IpInfo> getStaIpInfo() const;

    bool enableStaIpV6();
    IPv6Address staIPv6() const;

    std::optional<std::string> getHostname() const;
    bool setHostname( const std::string& hostname );

private:
    template <typename DelayType>
    WiFiGenericClass( const DelayType& shutdownDelay,
                      unsigned int _wifiProtocol = ( WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N ) )
    : ParentType( shutdownDelay )
    , m_wifiProtocol( _wifiProtocol ) {}

    std::optional<int32_t> channel();

    inline bool isStarted() const {
        return m_eventGroup.getStatusBits() & ESP_WIFI_STARTED_BIT;
    }
    bool start();
    bool stop();

    inline bool isStaMode() const {
        return getMode() & WIFI_MODE_STA;
    }
    bool enableSTA( const bool enable );

    inline bool isApMode() const {
        return getMode() & WIFI_MODE_AP;
    }
    bool enableAP( const bool enable );

    bool setSleep( bool enable );
    bool setSleep( wifi_ps_type_t mode );
    bool getSleep();

    bool setTxPower( wifi_power_t power );
    wifi_power_t getTxPower();

private:  // STA API
    bool tryJoinToLatestAp();

    bool tryJoinToAp( const std::string& ssid,
                      const std::string& passphrase = {},
                      int32_t channel = 0,
                      const std::optional<BssidType>& bssid = std::nullopt,
                      bool connect = true );

    bool disjoinFromAp( const std::string& accessPointName = std::string{}, bool _isEraseAp = false );

    inline bool isConnectedToAp() const {
        return m_eventGroup.getStatusBits() & ( STA_HAS_IP4_BIT | STA_HAS_IP6_BIT );
    }

    bool isStaStarted() const {
        return m_eventGroup.getStatusBits() & STA_STARTED_BIT;
    }

    //--------------------------------------------------------------------

    bool startAccessPoint( const std::string& ssid,
                           const std::string& passphrase = std::string(),
                           int channel = 1,
                           bool isSsidHidden = false,
                           unsigned int maxConnections = 4 );

    bool softApConfig( IPAddress local_ip, IPAddress gateway, IPAddress subnet );

    bool stopAccessPoint();

    bool isAccessPointStarted() const {
        return m_eventGroup.getStatusBits() & AP_STARTED_BIT;
    }

    inline bool isAccessPointHasClients() const {
        return 0 < m_numApClients;
    }

    //--------------------------------------------------------------------

    bool isWifiScanStarted() const {
        return m_eventGroup.getStatusBits() & WIFI_SCANNING_BIT;
    }

    bool startWifiScan();

    bool stopWifiScan();

private:
    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override {
        return ClbkRet::Continue;
    }

private:
    void visit( const bus::BusAddr from, const bus::BusAddr to, const command::JoinToLastAccessPoint& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, const command::JoinToAccessPoint& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, const command::DisjoinFromAccessPoint& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, const command::StartAccessPoint& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, const command::StopAccessPoint& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, const command::StartWifiScan& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, const command::StopWifiScan& cmd ) override;

#if CONFIG_BOARD_TYPE_PLUG_HUB
    void visit(::bus::BusAddr from, ::bus::BusAddr to, bus::command::GpioButtonLongClicked& command ) override;
#else   // not CONFIG_BOARD_TYPE_PLUG_HUB
    void visit(::bus::BusAddr from, ::bus::BusAddr to, bus::command::GpioButtonDoubleClicked& command ) override;
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB

private:
    // static int hostByName(const char *aHostname, IPAddress &aResult);

    wifi_mode_t getMode() const;

    bool destroyNetifInstance( esp_netif_t* _netif );

    void startHttpServer();

    bool shutdownHttpServer();

private:
    template <typename T>
    bool registerEventHandler( esp_event_base_t eventBase, int32_t eventId, T event_handler ) {
        return ESP_OK == esp_event_handler_register(
                             eventBase, eventId, reinterpret_cast<esp_event_handler_t>( event_handler ), this );
    }

    template <typename T>
    bool unregisterEventHandler( esp_event_base_t eventBase, int32_t eventId, T event_handler ) {
        return ESP_OK == esp_event_handler_unregister(
                             eventBase, eventId, reinterpret_cast<esp_event_handler_t>( event_handler ) );
    }

    bool tryConnect();

    inline bool isConnecting() const {
        return m_eventGroup.getStatusBits() & STA_CONNECTING_BIT;
    }

    // Wifi events handles

    static void onWifiEvent( WiFiGenericClass* self, esp_event_base_t eventBase, int32_t eventId, void* eventData );

    void onWifiEventFinishScanningAP( const wifi_event_sta_scan_done_t& event );

    void onWifiEventStationStarted();

    void onWifiEventStationStop();

    void onWifiEventStationConnectedToAp( const wifi_event_sta_connected_t& event );

    void onWifiEventDisconnectedFromAp( const wifi_event_sta_disconnected_t& event );

    void onWifiEventAuthModeOfApChanged( const wifi_event_sta_authmode_change_t& event );

    void onWifiEventSoftAccessPointStarted();

    void onWifiEventSoftAccessPointStoped();

    void onWifiEventConnectedToSoftAccessPoint( const wifi_event_ap_staconnected_t& event );

    void onWifiEventDisconnectedFromSoftAccessPoint( const wifi_event_ap_stadisconnected_t& event );

    void onWifiEventReceiveProbeRequestPacketInSoftAccessPointInterface( const wifi_event_ap_probe_req_rx_t& event );

    // Ip events handles

    static void onIpEvent( WiFiGenericClass* self, esp_event_base_t eventBase, int32_t eventId, void* eventData );

    void onIpEventGotIp( const ip_event_got_ip_t& event );

    void onIpEventStaLostIp( void* );

    void onIpEventApStaIpAssigned( const ip_event_ap_staipassigned_t& event );

    void onIpEventGotIp6( const ip_event_got_ip6_t& event );

    void onIpEventEthGotIp( const ip_event_got_ip_t& event );

    void onIpEventPppGotIp( const ip_event_got_ip_t& event );

    void onIpEventPppLostIp( void* );

private:
    const unsigned int m_wifiProtocol;

    bool m_wasConnectAttempt = false;

    sys::FreeRtosEventGroup m_eventGroup;
    esp_netif_t *m_staNetif = nullptr, *m_apNetIf = nullptr;

    unsigned int m_retryConnToApNum = 0;

    std::atomic<HttpServerState> m_httpServerState{ HttpServerState::Inactive };
    unsigned int m_numApClients{ 0 };
};

}  // namespace wifi_srv2
}  // namespace network
