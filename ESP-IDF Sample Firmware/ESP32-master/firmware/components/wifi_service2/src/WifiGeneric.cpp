#include "WifiGeneric.h"
#include "wifi_service2/Common.h"
#include "wifi_service2/WifiCommands.h"
#include "PrivateWifiCommands.h"
#include "WifiHttpServer.h"
#include "hub_core/Common.h"

#include "Ezlo.h"

extern "C" {
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <lwip/ip_addr.h>
#include <lwip/opt.h>
#include <lwip/err.h>
#include <lwip/dns.h>
#include <esp_ipc.h>
#include <esp_log.h>
#include <dhcpserver/dhcpserver.h>
#include <esp_netif.h>
}  // extern "C"

#include <vector>
#include <cstdint>
#include <cstdio>
#include <cinttypes>
#include <string>
#include <cstring>
#include <chrono>

#include "sdkconfig.h"

#define LOG_TAG "WiFiGenericClass"

#ifndef log_d
#define log_d( ... ) ESP_LOGD( LOG_TAG, __VA_ARGS__ )
#endif  // log_d

#ifndef log_i
#define log_i( ... ) ESP_LOGI( LOG_TAG, __VA_ARGS__ )
#endif  // log_i

#ifndef log_w
#define log_w( ... ) ESP_LOGW( LOG_TAG, __VA_ARGS__ )
#endif  // log_w

#ifndef log_e
#define log_e( ... ) ESP_LOGE( LOG_TAG, __VA_ARGS__ )
#endif  // log_e

namespace network {
namespace wifi_srv2 {

using namespace std::chrono_literals;

extern std::atomic_bool g_isConnectedToAccessPoint;

namespace {

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_WARN
const char* system_event_reasons[] = { "UNSPECIFIED",
                                       "AUTH_EXPIRE",
                                       "AUTH_LEAVE",
                                       "ASSOC_EXPIRE",
                                       "ASSOC_TOOMANY",
                                       "NOT_AUTHED",
                                       "NOT_ASSOCED",
                                       "ASSOC_LEAVE",
                                       "ASSOC_NOT_AUTHED",
                                       "DISASSOC_PWRCAP_BAD",
                                       "DISASSOC_SUPCHAN_BAD",
                                       "UNSPECIFIED",
                                       "IE_INVALID",
                                       "MIC_FAILURE",
                                       "4WAY_HANDSHAKE_TIMEOUT",
                                       "GROUP_KEY_UPDATE_TIMEOUT",
                                       "IE_IN_4WAY_DIFFERS",
                                       "GROUP_CIPHER_INVALID",
                                       "PAIRWISE_CIPHER_INVALID",
                                       "AKMP_INVALID",
                                       "UNSUPP_RSN_IE_VERSION",
                                       "INVALID_RSN_IE_CAP",
                                       "802_1X_AUTH_FAILED",
                                       "CIPHER_SUITE_REJECTED",
                                       "BEACON_TIMEOUT",
                                       "NO_AP_FOUND",
                                       "AUTH_FAIL",
                                       "ASSOC_FAIL",
                                       "HANDSHAKE_TIMEOUT",
                                       "CONNECTION_FAIL" };
#define reason2str( r ) ( ( r > 176 ) ? system_event_reasons[r - 176] : system_event_reasons[r - 1] )
#endif

bool isStaConfigEqual( const wifi_config_t& lhs, const wifi_config_t& rhs ) {
    return 0 == memcmp( &lhs, &rhs, sizeof( wifi_config_t ) );
}

bool isSoftApConfigEqual( const wifi_config_t& lhs, const wifi_config_t& rhs ) {
    if( strcmp( reinterpret_cast<const char*>( lhs.ap.ssid ), reinterpret_cast<const char*>( rhs.ap.ssid ) ) != 0 ) {
        return false;
    }
    if( strcmp( reinterpret_cast<const char*>( lhs.ap.password ), reinterpret_cast<const char*>( rhs.ap.password ) ) !=
        0 ) {
        return false;
    }
    if( lhs.ap.channel != rhs.ap.channel ) {
        return false;
    }
    if( lhs.ap.ssid_hidden != rhs.ap.ssid_hidden ) {
        return false;
    }
    if( lhs.ap.max_connection != rhs.ap.max_connection ) {
        return false;
    }
    return true;
}

}  // namespace

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Generic WiFi function
// -----------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

void WiFiGenericClass::onWifiEvent( WiFiGenericClass* self,
                                    esp_event_base_t eventBase,
                                    int32_t eventId,
                                    void* eventData ) {
    if( eventId == WIFI_EVENT_SCAN_DONE ) {
        auto event = reinterpret_cast<wifi_event_sta_scan_done_t*>( eventData );
        self->onWifiEventFinishScanningAP( *event );
    }
    else if( eventId == WIFI_EVENT_STA_START ) {
        self->onWifiEventStationStarted();
    }
    else if( eventId == WIFI_EVENT_STA_STOP ) {
        self->onWifiEventStationStop();
    }
    else if( eventId == WIFI_EVENT_STA_CONNECTED ) {
        auto event = reinterpret_cast<wifi_event_sta_connected_t*>( eventData );
        self->onWifiEventStationConnectedToAp( *event );
    }
    else if( eventId == WIFI_EVENT_STA_DISCONNECTED ) {
        auto event = reinterpret_cast<wifi_event_sta_disconnected_t*>( eventData );
        self->onWifiEventDisconnectedFromAp( *event );
    }
    else if( eventId == WIFI_EVENT_STA_AUTHMODE_CHANGE ) {
        auto event = reinterpret_cast<wifi_event_sta_authmode_change_t*>( eventData );
        self->onWifiEventAuthModeOfApChanged( *event );
    }
    //------------------------------------------------
    else if( eventId == WIFI_EVENT_AP_START ) {
        self->onWifiEventSoftAccessPointStarted();
    }
    else if( eventId == WIFI_EVENT_AP_STOP ) {
        self->onWifiEventSoftAccessPointStoped();
    }
    else if( eventId == WIFI_EVENT_AP_STACONNECTED ) {
        auto event = reinterpret_cast<wifi_event_ap_staconnected_t*>( eventData );
        self->onWifiEventConnectedToSoftAccessPoint( *event );
    }
    else if( eventId == WIFI_EVENT_AP_STADISCONNECTED ) {
        auto event = reinterpret_cast<wifi_event_ap_stadisconnected_t*>( eventData );
        self->onWifiEventDisconnectedFromSoftAccessPoint( *event );
    }
    else if( eventId == WIFI_EVENT_AP_PROBEREQRECVED ) {
        auto event = reinterpret_cast<wifi_event_ap_probe_req_rx_t*>( eventData );
        self->onWifiEventReceiveProbeRequestPacketInSoftAccessPointInterface( *event );
    }
    //-----------------------------------------------
    else {
        log_w( "Unknow wifi event: eventBase:%s, eventId:%d", eventBase, eventId );
    }
}

static void print_auth_mode( int authmode ) {
    switch( authmode ) {
    case WIFI_AUTH_OPEN:
        log_i( "Authmode \tWIFI_AUTH_OPEN" );
        break;
    case WIFI_AUTH_WEP:
        log_i( "Authmode \tWIFI_AUTH_WEP" );
        break;
    case WIFI_AUTH_WPA_PSK:
        log_i( "Authmode \tWIFI_AUTH_WPA_PSK" );
        break;
    case WIFI_AUTH_WPA2_PSK:
        log_i( "Authmode \tWIFI_AUTH_WPA2_PSK" );
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        log_i( "Authmode \tWIFI_AUTH_WPA_WPA2_PSK" );
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        log_i( "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE" );
        break;
    case WIFI_AUTH_WPA3_PSK:
        log_i( "Authmode \tWIFI_AUTH_WPA3_PSK" );
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        log_i( "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK" );
        break;
    default:
        log_i( "Authmode \tWIFI_AUTH_UNKNOWN" );
        break;
    }
}

static void print_cipher_type( int pairwise_cipher, int group_cipher ) {
    switch( pairwise_cipher ) {
    case WIFI_CIPHER_TYPE_NONE:
        log_i( "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE" );
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        log_i( "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40" );
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        log_i( "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104" );
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        log_i( "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP" );
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        log_i( "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP" );
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        log_i( "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP" );
        break;
    default:
        log_i( "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN" );
        break;
    }

    switch( group_cipher ) {
    case WIFI_CIPHER_TYPE_NONE:
        log_i( "Group Cipher \tWIFI_CIPHER_TYPE_NONE" );
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        log_i( "Group Cipher \tWIFI_CIPHER_TYPE_WEP40" );
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        log_i( "Group Cipher \tWIFI_CIPHER_TYPE_WEP104" );
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        log_i( "Group Cipher \tWIFI_CIPHER_TYPE_TKIP" );
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        log_i( "Group Cipher \tWIFI_CIPHER_TYPE_CCMP" );
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        log_i( "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP" );
        break;
    default:
        log_i( "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN" );
        break;
    }
}

void WiFiGenericClass::onWifiEventFinishScanningAP( const wifi_event_sta_scan_done_t& event ) {
    log_i( "onWifiEventFinishScanningAP" );

    m_eventGroup.clearStatusBits( WIFI_SCANNING_BIT );

    if( ESP_OK != event.status ) {
        log_e( "Scan wifi failed" );
        return;
    }
    log_i( "Scan completed successfully, founded %d access points", event.number );

    WifiApInfoVecType apInfo( event.number, WifiApInfo{} );
    uint16_t number = event.number;
    if( ESP_OK != esp_wifi_scan_get_ap_records( &number, apInfo.data() ) ) {
        log_e( "Can't get access points records" );
        send<responce::DiscoveredWifiAccessPoints>( responce::BUS_NAME );
    }

    // Filter wifi repeaters with max signal
    WifiApInfoMapType apMap;
    for( auto& ap : apInfo ) {
        auto ssidStr = ap.ssidToString();
        auto itr = apMap.find( ssidStr );
        if( apMap.end() == itr ) {
            apMap.insert( { std::move( ssidStr ), std::move( ap ) } );
        }
        else {
            if( itr->second.rssi < ap.rssi ) {
                itr->second = std::move( ap );
            }
        }
    }
    WifiApInfoSetType apSet;
    for( const auto& [ssid, ap] : apMap ) {
        apSet.insert( ap );
    }
    send<responce::DiscoveredWifiAccessPoints>( responce::BUS_NAME, std::move( apSet ) );
}

void WiFiGenericClass::onWifiEventStationStarted() {
    log_i( "onWifiEventStationStarted" );

    m_eventGroup.setStatusBits( STA_STARTED_BIT ) && tryConnect();
}

void WiFiGenericClass::onWifiEventStationStop() {
    log_i( "onWifiEventStationStop" );

    m_eventGroup.clearStatusBits( STA_STARTED_BIT | STA_CONNECTED_BIT | STA_HAS_IP4_BIT | STA_HAS_IP6_BIT |
                                  STA_CONNECTING_BIT );
}

void WiFiGenericClass::onWifiEventStationConnectedToAp( const wifi_event_sta_connected_t& event ) {
    log_i( "onWifiEventStationConnectedToAp" );
    m_eventGroup.setStatusBits( STA_CONNECTED_BIT );
    send<responce::ConnectedToWifiStation>( responce::BUS_NAME );

    if( !isAccessPointHasClients() ) {
        stopAccessPoint();
    }
}

void WiFiGenericClass::onWifiEventDisconnectedFromAp( const wifi_event_sta_disconnected_t& event ) {
    uint8_t reason = event.reason;
    log_w( "onWifiEventDisconnectedFromAp, reason: %u - %s", reason, reason2str( reason ) );
    g_isConnectedToAccessPoint = false;
    m_eventGroup.clearStatusBits( STA_CONNECTED_BIT | STA_HAS_IP4_BIT | STA_HAS_IP6_BIT | STA_CONNECTING_BIT );

    send<responce::DisconnectedFromWifiStation>( responce::BUS_NAME );

    if( isShutdownStart() || isShutdowned() || !hasValidStaConfig() || isAccessPointHasClients() ) {
        return;
    }

    auto reconClbk = [this]() {
        if( isConnectedToAp() || isAccessPointHasClients() ) {
            return;
        }
        if( !enableSTA( true ) ) {
            return;
        }

        if( isStaStarted() ) {
            tryConnect();
        }

        m_retryConnToApNum += 1;
        log_i( "retry to connect to the AP #%d", m_retryConnToApNum );
    };

    if( m_retryConnToApNum < CONFIG_WIFI_SERVICE_CONN_STA_MAXIMUM_RETRY ) {
        setDelay( 10s, reconClbk );
        return;
    }

    if( m_retryConnToApNum == CONFIG_WIFI_SERVICE_CONN_STA_MAXIMUM_RETRY && !isAccessPointStarted() ) {
        startAccessPoint( core::hubName() );
    }

    setDelay( 40s, reconClbk );
}

void WiFiGenericClass::onWifiEventAuthModeOfApChanged( const wifi_event_sta_authmode_change_t& event ) {
    log_i( "onWifiEventAuthModeOfApChanged" );
}

void WiFiGenericClass::onWifiEventSoftAccessPointStarted() {
    log_i( "onWifiEventSoftAccessPointStarted" );
    m_eventGroup.setStatusBits( AP_STARTED_BIT );

    startHttpServer();
}

void WiFiGenericClass::onWifiEventSoftAccessPointStoped() {
    log_i( "onWifiEventSoftAccessPointStoped" );
    m_eventGroup.clearStatusBits( AP_STARTED_BIT );

    m_numApClients = 0;
    shutdownHttpServer();

    if( !m_wasConnectAttempt ) {
        log_i( "AP was down unexpectly, device will be reboot" );
        core::Ezlo::get()->reboot();
    }
}

void WiFiGenericClass::startHttpServer() {
    log_i( "startHttpServer" );
    if( HttpServerState::Inactive != m_httpServerState ) {
        log_e( "Can't start wifi http server" );
        return;
    }

    core::Thread httpServerThread = runWifiHttpServerThread( m_httpServerState );
    if( !httpServerThread ) {
        log_e( "Can't start http thread" );
    }
    httpServerThread.detach();
    log_i( "wifi http server started" );
}

bool WiFiGenericClass::shutdownHttpServer() {
    log_i( "shutdownHttpServer" );

    if( HttpServerState::Inactive == m_httpServerState ) {
        log_w( "Wifi http srv closed" );
        return true;
    }

    if( HttpServerState::Run == m_httpServerState ) {
        m_httpServerState = HttpServerState::Shutdowned;
    }

    while( m_httpServerState != HttpServerState::Inactive ) {
        core::this_thread::sleep_for( 250ms );
    }

    log_i( "Wifi http srv closed" );
    return true;
}

bool WiFiGenericClass::tryConnect() {
    /*if( isConnecting() ) {
        log_w( "esp_wifi_connect() in process" );
        return true;
    }*/

    if( !hasValidStaConfig() ) {
        log_w( "invalid sta config" );
        return false;
    }

    if( ESP_OK == esp_wifi_connect() ) {
        m_eventGroup.setStatusBits( STA_CONNECTING_BIT );
        log_i( "esp_wifi_connect() success" );
        return true;
    }

    log_w( "esp_wifi_connect() failed" );
    return false;
}

void WiFiGenericClass::onWifiEventConnectedToSoftAccessPoint( const wifi_event_ap_staconnected_t& event ) {
    log_i( "onWifiEventConnectedToSoftAccessPoint(station:" MACSTR ", AID=%d), m_numApClients=%d",
           MAC2STR( event.mac ),
           event.aid,
           m_numApClients );

    m_numApClients += 1;
    m_wasConnectAttempt = true;
}

void WiFiGenericClass::onWifiEventDisconnectedFromSoftAccessPoint( const wifi_event_ap_stadisconnected_t& event ) {
    log_i( "onWifiEventDisconnectedFromSoftAccessPoint(station:" MACSTR ", AID:%d), m_numApClients=%d",
           MAC2STR( event.mac ),
           event.aid,
           m_numApClients );

    if( 0 < m_numApClients ) {
        m_numApClients -= 1;
    }
}

void WiFiGenericClass::onWifiEventReceiveProbeRequestPacketInSoftAccessPointInterface(
    const wifi_event_ap_probe_req_rx_t& event ) {
    log_i( "onWifiEventReceiveProbeRequestPacketInSoftAccessPointInterface" );
}

void WiFiGenericClass::onIpEvent( WiFiGenericClass* self,
                                  esp_event_base_t eventBase,
                                  int32_t eventId,
                                  void* eventData ) {
    switch( eventId ) {
    case IP_EVENT_STA_GOT_IP:
        self->onIpEventGotIp( *reinterpret_cast<ip_event_got_ip_t*>( eventData ) );
        break;
    case IP_EVENT_STA_LOST_IP:
        self->onIpEventStaLostIp( eventData );
        break;
    case IP_EVENT_AP_STAIPASSIGNED:
        self->onIpEventApStaIpAssigned( *reinterpret_cast<ip_event_ap_staipassigned_t*>( eventData ) );
        break;
    case IP_EVENT_GOT_IP6:
        self->onIpEventGotIp6( *reinterpret_cast<ip_event_got_ip6_t*>( eventData ) );
        break;
    case IP_EVENT_ETH_GOT_IP:
        self->onIpEventEthGotIp( *reinterpret_cast<ip_event_got_ip_t*>( eventData ) );
        break;
    case IP_EVENT_PPP_GOT_IP:
        self->onIpEventPppGotIp( *reinterpret_cast<ip_event_got_ip_t*>( eventData ) );
        break;
    case IP_EVENT_PPP_LOST_IP:
        self->onIpEventPppLostIp( eventData );
        break;
    default:
        log_w( "Unknow ip event: eventBase:%s, eventId:%d", eventBase, eventId );
    }
}

void WiFiGenericClass::onIpEventGotIp( const ip_event_got_ip_t& event ) {
    log_i( "onIpEventGotIp: ip=" IPSTR, IP2STR( &event.ip_info.ip ) );

    m_retryConnToApNum = 0;
    g_isConnectedToAccessPoint = true;
    m_eventGroup.setStatusBits( STA_HAS_IP4_BIT );
    m_eventGroup.clearStatusBits( STA_CONNECTING_BIT );
}

void WiFiGenericClass::onIpEventStaLostIp( void* ) {
    log_i( "onIpEventStaLostIp" );
}

void WiFiGenericClass::onIpEventApStaIpAssigned( const ip_event_ap_staipassigned_t& event ) {
    log_i( "onIpEventApStaIpAssigned: ip=" IPSTR, IP2STR( &event.ip ) );
}

void WiFiGenericClass::onIpEventGotIp6( const ip_event_got_ip6_t& event ) {
    log_i( "onIpEventGotIp6: ip=" IPV6STR, IPV62STR( event.ip6_info.ip ) );

    m_retryConnToApNum = 0;
    g_isConnectedToAccessPoint = true;
    m_eventGroup.setStatusBits( STA_HAS_IP6_BIT );
    m_eventGroup.clearStatusBits( STA_CONNECTING_BIT );
}

void WiFiGenericClass::onIpEventEthGotIp( const ip_event_got_ip_t& event ) {
    log_w( "onIpEventEthGotIp: ip=" IPSTR, IP2STR( &event.ip_info.ip ) );
}

void WiFiGenericClass::onIpEventPppGotIp( const ip_event_got_ip_t& event ) {
    log_w( "onIpEventPppGotIp: ip=" IPSTR, IP2STR( &event.ip_info.ip ) );
}

void WiFiGenericClass::onIpEventPppLostIp( void* ) {
    log_w( "onIpEventPppLostIp" );
}

bool WiFiGenericClass::initializeImpl() {
    if( !core::Shutdownable::initializeImpl() ) {
        log_e( "Can't initialize IBusClient" );
        return false;
    }

    if( !bus::Bus::get().join( command::BUS_NAME, *this ) ) {
        log_e( "Can't join to %s bus name", command::BUS_NAME );
        return false;
    }

    if( !bus::Bus::get().join( bus::command::GPIO_BUTTON_BUS_CLIENT_NAME, *this ) ) {
        log_e( "Can't join to %s bus namespace.", bus::command::GPIO_BUTTON_BUS_CLIENT_NAME );
        return false;
    }

    m_eventGroup.setStatusBits( WIFI_DNS_IDLE_BIT );

    esp_err_t errCode;

    errCode = esp_netif_init();
    if( ESP_OK != errCode ) {
        log_e( "esp_netif_init() failed: err=%d", errCode );
        return false;
    }

    errCode = esp_event_loop_create_default();
    if( ESP_OK != errCode ) {
        log_e( "esp_event_loop_create_default() failed: err=%d", errCode );
        return false;
    }

    if( !registerEventHandler( WIFI_EVENT, ESP_EVENT_ANY_ID, &onWifiEvent ) ) {
        log_e( "register event handler onWifiEvent failed!" );
        return false;
    }

    if( !registerEventHandler( IP_EVENT, ESP_EVENT_ANY_ID, &onIpEvent ) ) {
        log_e( "register event handler onIpEvent failed!" );
        return false;
    }

    wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    errCode = esp_wifi_init( &wifiCfg );
    if( ESP_OK != errCode ) {
        log_e( "esp_wifi_init() failed: err=%d", errCode );
        return false;
    }

    {
        auto err = esp_wifi_set_mode( WIFI_MODE_APSTA );
        if( ESP_OK != err ) {
            log_e( "Could not set mode! %d", err );
            return false;
        }

        err = esp_wifi_set_protocol( WIFI_IF_STA, m_wifiProtocol );
        if( ESP_OK != err ) {
            log_e( "Can't set STA protocol, err=%d", err );
            return false;
        }
        err = esp_wifi_set_protocol( WIFI_IF_AP, m_wifiProtocol );
        if( ESP_OK != err ) {
            log_e( "Can't set AP protocol, err=%d", err );
            return false;
        }

        m_staNetif = esp_netif_create_default_wifi_sta();
        if( !m_staNetif ) {
            log_e( "Could not create STA network interface" );
            return false;
        }
        m_apNetIf = esp_netif_create_default_wifi_ap();
        if( !m_apNetIf ) {
            log_e( "Could not create AP network interface" );
            return false;
        }

        err = esp_wifi_set_mode( WIFI_MODE_NULL );
        if( ESP_OK != err ) {
            log_e( "Could not set mode! %d", err );
            return false;
        }
    }

    return true;
}

WiFiGenericClass::~WiFiGenericClass() {
    shutdownHttpServer();

    if( !stop() ) {
        log_e( "Couldn't stop wifi" );
        return;
    }

    if( !unregisterEventHandler( IP_EVENT, ESP_EVENT_ANY_ID, &onIpEvent ) ) {
        log_e( "Couldn't unregister onIpEvent" );
    }

    if( !unregisterEventHandler( WIFI_EVENT, ESP_EVENT_ANY_ID, &onWifiEvent ) ) {
        log_e( "Couldn't unregister onWifiEvent" );
    }

    if( ESP_OK != esp_wifi_deinit() ) {
        log_e( "Couldn't deinit wifi" );
    }
}

std::optional<IpInfo> WiFiGenericClass::getStaIpInfo() const {
    IpInfo info = {};
    return ESP_OK == esp_netif_get_ip_info( m_staNetif, &info ) ? std::make_optional<IpInfo>( info ) : std::nullopt;
}

bool WiFiGenericClass::enableStaIpV6() {
    return ESP_OK == esp_netif_create_ip6_linklocal( m_staNetif );
}

IPv6Address WiFiGenericClass::staIPv6() const {
    esp_ip6_addr_t addr;
    return ESP_OK == esp_netif_get_ip6_linklocal( m_staNetif, &addr ) ? IPv6Address( addr ) : IPv6Address();
}

std::optional<std::string> WiFiGenericClass::getHostname() const {
    const char* hostname = nullptr;
    return ESP_OK == esp_netif_get_hostname( m_staNetif, &hostname ) && nullptr != hostname ?
               std::make_optional( std::string( hostname ) ) :
               std::nullopt;
}

bool WiFiGenericClass::setHostname( const std::string& hostname ) {
    return ESP_OK == esp_netif_set_hostname( m_staNetif, hostname.c_str() );
}

bool WiFiGenericClass::start() {
    log_i( "start()" );

    if( isStarted() ) {
        return true;
    }

    esp_err_t err = esp_wifi_start();
    if( ESP_OK != err ) {
        log_e( "esp_wifi_start error: %d", err );
        return false;
    }

    log_i( "esp_wifi_start() successfull" );
    m_eventGroup.setStatusBits( ESP_WIFI_STARTED_BIT );
    return true;
}

bool WiFiGenericClass::stop() {
    log_i( "stop()" );

    if( !isStarted() ) {
        return true;
    }

    auto err = esp_wifi_stop();
    if( ESP_OK != err ) {
        log_e( "Could not stop WiFi! %d", err );
        return false;
    }

    log_d( "esp_wifi_stop successfull" );
    m_eventGroup.clearStatusBits( ESP_WIFI_STARTED_BIT );
    return true;
}

std::optional<int32_t> WiFiGenericClass::channel() {
    uint8_t primaryChan = 0;
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    return ESP_OK == esp_wifi_get_channel( &primaryChan, &secondChan ) ? std::make_optional( primaryChan ) :
                                                                         std::nullopt;
}

wifi_mode_t WiFiGenericClass::getMode() const {
    if( !isStarted() ) {
        return WIFI_MODE_NULL;
    }

    wifi_mode_t mode;
    auto res = esp_wifi_get_mode( &mode );
    if( ESP_OK == res ) {
        return mode;
    }
    else if( ESP_ERR_WIFI_NOT_INIT == res ) {
        log_w( "WiFi is not initialized by esp_wifi_init" );
    }
    else if( ESP_ERR_INVALID_ARG == res ) {
        log_w( "invalid argument" );
    }

    return WIFI_MODE_NULL;
}

bool WiFiGenericClass::enableSTA( const bool enable ) {
    log_i( "enableSTA(enable:%d)", (int)enable );

    if( isStaMode() == enable ) {
        log_w( "Already in sta mode" );
        return true;
    }

    {
        auto oldMode = getMode();
        auto newMode = enable ? wifi_mode_t( oldMode | WIFI_MODE_STA ) : wifi_mode_t( oldMode & ( ~WIFI_MODE_STA ) );
        auto err = esp_wifi_set_mode( newMode );
        if( ESP_OK != err ) {
            log_e( "Could not set mode! %d", err );
            return false;
        }
        else {
            log_i( "Set mode to %d", (int)newMode );
        }

        if( WIFI_MODE_NULL == newMode ) {
            return stop();
        }
    }

    return start();
}

bool WiFiGenericClass::enableAP( const bool enable ) {
    log_i( "enableAP(enable:%d)", (int)enable );

    if( isApMode() == enable ) {
        log_w( "Already in ap mode" );
        return true;
    }

    {
        auto oldMode = getMode();
        auto newMode = enable ? wifi_mode_t( oldMode | WIFI_MODE_AP ) : wifi_mode_t( oldMode & ( ~WIFI_MODE_AP ) );
        auto err = esp_wifi_set_mode( newMode );
        if( ESP_OK != err ) {
            log_e( "Could not set mode! %d", err );
            return false;
        }
        else {
            log_i( "Set mode to %d", newMode );
        }

        if( WIFI_MODE_NULL == newMode ) {
            return stop();
        }
    }

    return start();
}

bool WiFiGenericClass::setSleep( bool enable ) {
    if( !isStaMode() ) {
        log_w( "STA has not been started" );
        return false;
    }
    return esp_wifi_set_ps( enable ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE ) == ESP_OK;
}

bool WiFiGenericClass::setSleep( wifi_ps_type_t mode ) {
    if( !isStaMode() ) {
        log_w( "STA has not been started" );
        return false;
    }
    return esp_wifi_set_ps( mode ) == ESP_OK;
}

bool WiFiGenericClass::getSleep() {
    wifi_ps_type_t ps;
    if( !isStaMode() ) {
        log_w( "STA has not been started" );
        return false;
    }

    if( esp_wifi_get_ps( &ps ) == ESP_OK ) {
        return ps == WIFI_PS_MIN_MODEM;
    }
    return false;
}

bool WiFiGenericClass::setTxPower( wifi_power_t power ) {
    if( ( m_eventGroup.getStatusBits() & ( STA_STARTED_BIT | AP_STARTED_BIT ) ) == 0 ) {
        log_w( "Neither AP or STA has been started" );
        return false;
    }
    return esp_wifi_set_max_tx_power( power ) == ESP_OK;
}

wifi_power_t WiFiGenericClass::getTxPower() {
    int8_t power;
    if( ( m_eventGroup.getStatusBits() & ( STA_STARTED_BIT | AP_STARTED_BIT ) ) == 0 ) {
        log_w( "Neither AP or STA has been started" );
        return WIFI_POWER_19_5dBm;
    }
    if( esp_wifi_get_max_tx_power( &power ) ) {
        return WIFI_POWER_19_5dBm;
    }
    return (wifi_power_t)power;
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Generic Network function
// ---------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

/**
 * DNS callback
 * @param name
 * @param ipaddr
 * @param callback_arg
 */
/*static void wifi_dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if(ipaddr) {
        (*reinterpret_cast<IPAddress*>(callback_arg)) = ipaddr->u_addr.ip4.addr;
    }
    xEventGroupSetBits(m_eventGroup, WIFI_DNS_DONE_BIT);
}*/

/**
 * Resolve the given hostname to an IP address.
 * @param aHostname     Name to be resolved
 * @param aResult       IPAddress structure to store the returned IP address
 * @return 1 if aIPAddrString was successfully converted to an IP address,
 *          else error code
 */
/*int WiFiGenericClass::hostByName(const char* aHostname, IPAddress& aResult)
{
    ip_addr_t addr;
    aResult = static_cast<uint32_t>(0);
    m_eventGroup.waitStatusBits(WIFI_DNS_IDLE_BIT, 3s);
    m_eventGroup.clearStatusBits(WIFI_DNS_IDLE_BIT | WIFI_DNS_DONE_BIT);
    err_t err = dns_gethostbyname(aHostname, &addr, &wifi_dns_found_callback, &aResult);
    if(err == ERR_OK && addr.u_addr.ip4.addr) {
        aResult = addr.u_addr.ip4.addr;
    } else if(err == ERR_INPROGRESS) {
        m_eventGroup.waitStatusBits(WIFI_DNS_DONE_BIT, 2s);  //real internal timeout in lwip library is 14[s]
        m_eventGroup.clearStatusBits(WIFI_DNS_DONE_BIT);
    }
    m_eventGroup.setStatusBits(WIFI_DNS_IDLE_BIT);
    if((uint32_t)aResult == 0){
        log_e("DNS Failed for %s", aHostname);
    }
    return (uint32_t)aResult != 0;
}*/

bool WiFiGenericClass::tryJoinToLatestAp() {
    log_i( "tryJoinToLatestAp()" );

    if( !enableSTA( true ) ) {
        log_e( "STA enable failed!" );
        return false;
    }

    const bool isSuccess = !isStaStarted() || tryConnect();
    if( isSuccess ) {
        log_i( "tryJoinToLatestAp successfull" );
    }
    return isSuccess;
}

bool WiFiGenericClass::tryJoinToAp( const std::string& ssid,
                                    const std::string& passphrase,
                                    int32_t channel,
                                    const std::optional<BssidType>& bssid,
                                    bool connect ) {
    log_i( "tryJoinToAp: ssid=%s, passw=%s", ssid.c_str(), passphrase.c_str() );

    if( !enableSTA( true ) ) {
        log_e( "STA enable failed!" );
        return false;
    }

    if( ssid.empty() || 0x00 == ssid[0] || ssid.length() > 31 ) {
        log_e( "SSID too long or missing!" );
        return false;
    }

    if( !passphrase.empty() && passphrase.length() > 64 ) {
        log_e( "passphrase too long!" );
        return false;
    }

    wifi_config_t conf = {};
    std::strcpy( reinterpret_cast<char*>( conf.sta.ssid ), ssid.c_str() );

    if( !passphrase.empty() ) {
        if( 64 == passphrase.length() ) {  // it's not a passphrase, is the PSK
            std::memcpy( reinterpret_cast<char*>( conf.sta.password ), passphrase.c_str(), 64 );
        }
        else {
            std::strcpy( reinterpret_cast<char*>( conf.sta.password ), passphrase.c_str() );
        }
    }

    if( bssid ) {
        conf.sta.bssid_set = 1;
        std::memcpy( &conf.sta.bssid[0], bssid->data(), 6 );
    }

    if( channel > 0 && channel <= 13 ) {
        conf.sta.channel = channel;
    }

    wifi_config_t current_conf;
    if( ESP_OK != esp_wifi_get_config( WIFI_IF_STA, &current_conf ) ) {
        return false;
    }
    log_i( "Current ssid:%s, passw:%s", (const char*)current_conf.sta.ssid, (const char*)current_conf.sta.ssid );

    if( !isStaConfigEqual( current_conf, conf ) ) {
        log_i( "configs not equal" );
        if( ESP_OK != esp_wifi_disconnect() ) {
            log_e( "disconnect failed!" );
            return false;
        }

        if( ESP_OK != esp_wifi_set_config( WIFI_IF_STA, &conf ) ) {
            return false;
        }
        else {
            log_i( "esp_wifi_set_config success" );
        }
    }
    /*else if(status() == WL_CONNECTED){
        return true;
    }*/
    else {
        log_i( "configs equal" );
        esp_wifi_set_config( WIFI_IF_STA, &conf );
    }

    return !connect || !isStaStarted() || tryConnect();
}

bool WiFiGenericClass::disjoinFromAp( const std::string& accessPointName, bool _isEraseAp ) {
    bool isSuccess = true;

    if( isStaMode() ) {
        if( ESP_OK != esp_wifi_disconnect() ) {
            log_e( "Can't perform wifi disconnect" );
            isSuccess = false;
        }

        if( _isEraseAp && !wipeStaConfig() ) {
            log_e( "clear config failed!" );
            isSuccess = false;
        }
    }

    if( !accessPointName.empty() && !startAccessPoint( accessPointName ) ) {
        isSuccess = false;
    }

    return isSuccess;
}

bool WiFiGenericClass::startAccessPoint( const std::string& ssid,
                                         const std::string& passphrase,
                                         int channel,
                                         bool isSsidHidden,
                                         unsigned int maxConnections ) {
    log_i( "startAccessPoint(ssid=%s)", ssid.c_str() );

    if( ssid.empty() ) {
        log_e( "SSID missing" );
        return false;
    }

    if( !enableAP( true ) ) {
        log_e( "Can't enable AP" );
        return false;
    }

    if( !passphrase.empty() && 8 > passphrase.length() ) {
        log_e( "Passphrase to short" );
        return false;
    }

    wifi_config_t conf = {};
    strlcpy( reinterpret_cast<char*>( conf.ap.ssid ), ssid.c_str(), sizeof( conf.ap.ssid ) );
    conf.ap.channel = channel;
    conf.ap.ssid_len = strlen( reinterpret_cast<char*>( conf.ap.ssid ) );
    conf.ap.ssid_hidden = isSsidHidden ? 1 : 0;
    conf.ap.max_connection = maxConnections;
    conf.ap.beacon_interval = 100;

    if( passphrase.empty() || 0 == passphrase.length() ) {
        conf.ap.authmode = WIFI_AUTH_OPEN;
        *conf.ap.password = 0;
    }
    else {
        conf.ap.authmode = WIFI_AUTH_WPA2_PSK;
        strlcpy( reinterpret_cast<char*>( conf.ap.password ), passphrase.c_str(), sizeof( conf.ap.password ) );
    }

    wifi_config_t conf_current;
    if( ESP_OK != esp_wifi_get_config( WIFI_IF_AP, &conf_current ) ) {
        return false;
    }

    if( !isSoftApConfigEqual( conf, conf_current ) ) {
        if( ESP_OK == esp_wifi_set_config( ESP_IF_WIFI_AP, &conf ) ) {
            log_i( "Set ap config" );
        }
        else {
            log_e( "Can't set ap config" );
            return false;
        }
    }

    /*if( ESP_ERR_TCPIP_ADAPTER_DHCPC_START_FAILED == esp_netif_dhcps_start(m_apNetIf)){
        log_e("-=-=-==-= dhcp client start failed!");
        return false;
    }
    else {
        log_i("-=-=-=-=-=-== dhcp client started!");
    }*/

    return true;
}

bool WiFiGenericClass::softApConfig( IPAddress local_ip, IPAddress gateway, IPAddress subnet ) {
    if( !enableAP( true ) ) {
        log_e( "Enable AP failed" );
        return false;
    }

    if( ESP_OK != esp_netif_dhcps_stop( m_apNetIf ) ) {
        return false;
    }

    esp_netif_ip_info_t info = {};
    info.ip.addr = static_cast<uint32_t>( local_ip );
    info.gw.addr = static_cast<uint32_t>( gateway );
    info.netmask.addr = static_cast<uint32_t>( subnet );
    if( ESP_OK != esp_netif_set_ip_info( m_apNetIf, &info ) ) {
        return false;
    }

    dhcps_lease_t lease = {};
    lease.enable = true;
    lease.start_ip.addr = static_cast<uint32_t>( local_ip ) + ( 1 << 24 );
    lease.end_ip.addr = static_cast<uint32_t>( local_ip ) + ( 11 << 24 );

    if( ESP_OK !=
        esp_netif_dhcps_option(
            m_apNetIf, ESP_NETIF_OP_SET, ESP_NETIF_REQUESTED_IP_ADDRESS, (void*)&lease, sizeof( dhcps_lease_t ) ) ) {
        return false;
    }

    return ESP_OK == esp_netif_dhcps_start( m_apNetIf );
}

bool WiFiGenericClass::stopAccessPoint() {
    log_i( "stopAccessPoint" );
    return enableAP( false );
}

bool WiFiGenericClass::startWifiScan() {
    if( isConnecting() ) {
        return false;
    }

    if( isWifiScanStarted() ) {
        log_e( "Wifi scan already started" );
        return true;
    }

    if( !isStaMode() && !enableSTA( true ) ) {
        log_e( "Can't enable STA mode" );
        return false;
    }

    if( !m_eventGroup.waitStatusBits( STA_STARTED_BIT, 2s ) ) {
        log_e( "Can't start wifi scan because STA not started" );
        return false;
    }

    // Why we should use probes here:
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#scan-when-wi-fi-is-connecting
    unsigned int probesCount = 0;
    constexpr unsigned int maxProbesCount = 5;
    while( maxProbesCount > ( ++probesCount ) ) {
        constexpr auto isBlocked = false;
        if( ESP_OK == esp_wifi_scan_start( nullptr, isBlocked ) ) {
            return m_eventGroup.setStatusBits( WIFI_SCANNING_BIT );
        }
        core::this_thread::sleep_for( 1s );
    }

    log_e( "Can't start wifi scan" );
    return false;
}

bool WiFiGenericClass::stopWifiScan() {
    return false;
}

void WiFiGenericClass::visit( const bus::BusAddr from,
                              const bus::BusAddr to,
                              const command::JoinToLastAccessPoint& cmd ) {
    tryJoinToLatestAp();
}

void WiFiGenericClass::visit( const bus::BusAddr from, const bus::BusAddr to, const command::JoinToAccessPoint& cmd ) {
    tryJoinToAp( cmd.ssid(), cmd.passw(), cmd.channel(), cmd.bssid() );
}

void WiFiGenericClass::visit( const bus::BusAddr from,
                              const bus::BusAddr to,
                              const command::DisjoinFromAccessPoint& cmd ) {
    disjoinFromAp( cmd.accessPointName(), cmd.isEraseAp() );
}

void WiFiGenericClass::visit( const bus::BusAddr from, const bus::BusAddr to, const command::StartAccessPoint& cmd ) {
    startAccessPoint( cmd.ssid(), cmd.passw(), cmd.channel(), cmd.isSsidHidden(), cmd.maxConnections() );
}

void WiFiGenericClass::visit( const bus::BusAddr from, const bus::BusAddr to, const command::StopAccessPoint& cmd ) {
    stopAccessPoint();
}

void WiFiGenericClass::visit( const bus::BusAddr from, const bus::BusAddr to, const command::StartWifiScan& cmd ) {
    startWifiScan();
}

void WiFiGenericClass::visit( const bus::BusAddr from, const bus::BusAddr to, const command::StopWifiScan& cmd ) {
    stopWifiScan();
}

#if CONFIG_BOARD_TYPE_PLUG_HUB
void WiFiGenericClass::visit(::bus::BusAddr from, ::bus::BusAddr to, bus::command::GpioButtonLongClicked& command ) {
    log_i( "Reset WiFi" );
    constexpr bool shouldWipeApConfig = true;
    disjoinFromAp( core::hubName(), shouldWipeApConfig );
}
#else   // not CONFIG_BOARD_TYPE_PLUG_HUB
void WiFiGenericClass::visit(::bus::BusAddr from, ::bus::BusAddr to, bus::command::GpioButtonDoubleClicked& command ) {
    log_i( "Reset WiFi" );
    constexpr bool shouldWipeApConfig = true;
    disjoinFromAp( core::hubName(), shouldWipeApConfig );
}
#endif  // CONFIG_BOARD_TYPE_PLUG_HUB

}  // namespace wifi_srv2
}  // namespace network
