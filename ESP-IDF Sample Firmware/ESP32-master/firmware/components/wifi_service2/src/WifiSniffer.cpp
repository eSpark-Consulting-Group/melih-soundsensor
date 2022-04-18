#include "WifiSniffer.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
#include "wifi_service2/WifiTypes.h"
#include "wifi_service2/Common.h"

#include "system/Queue.h"
#include "Thread.h"

#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <cstring>
#include <algorithm>

#include "DebugUtils.h"

namespace network {
namespace wifi_srv2 {

using namespace std::chrono_literals;

static const char* SNIFFER_TAG = "WifiSniffer";
const char* WifiSniffer::BUS_NAME = "WifiSniffer";

struct sniffer_packet_info_t {
    sniffer_packet_info_t() = default;

    sniffer_packet_info_t( const BssidType& addr, const BssidType& bssid, const int rssi )
    : m_addr{ addr }
    , m_bssid{ bssid }
    , m_rssi{ rssi } {
        m_ssid[0] = '\0';
    }

    sniffer_packet_info_t( const char* ssid,
                           const unsigned int ssidLen,
                           const BssidType& addr,
                           const BssidType& bssid,
                           const int rssi )
    : m_addr{ addr }
    , m_bssid{ bssid }
    , m_rssi{ rssi } {
        const auto bytesToCopy = std::min<unsigned int>( sizeof( m_ssid ) - 1, ssidLen );
        std::memcpy( m_ssid.data(), ssid, bytesToCopy );
        m_ssid[bytesToCopy] = '\0';
    }

    BssidType m_addr;
    BssidType m_bssid;
    Rssi m_rssi;
    std::array<char, 34> m_ssid;
};

struct sniffer_runtime_t {
    sniffer_runtime_t( const sniffer_intf_t intf )
    : interf{ intf } {
        if( isValid() ) {
            g_instance = this;
        }
    }

    ~sniffer_runtime_t() {
        g_instance = nullptr;
    }

    template <typename ClbkType>
    bool startWiFiPromicuousMode( ClbkType clbk ) {
        wifi_promiscuous_filter_t wifi_filter;

        // Start WiFi Promicuous Mode
        wifi_filter.filter_mask = filter;
        esp_wifi_set_promiscuous_filter( &wifi_filter );
        esp_wifi_set_promiscuous_rx_cb( reinterpret_cast<wifi_promiscuous_cb_t>( clbk ) );
        if( ESP_OK != esp_wifi_set_promiscuous( true ) ) {
            ESP_LOGI( SNIFFER_TAG, "start wifi promiscuous failed" );
            return false;
        }

        esp_wifi_set_channel( CONFIG_WIFI_SERVICE_SNIFFER_CHANNEL, WIFI_SECOND_CHAN_NONE );
        ESP_LOGI( SNIFFER_TAG, "start WiFi promiscuous ok" );
        return true;
    }

    bool stopWiFiPromicuousMode() {
        bool isSuccess = ESP_OK == esp_wifi_set_promiscuous( false );
        core::this_thread::sleep_for( 500ms );
        return isSuccess;
    }

    bool isValid() const {
        return m_queue.isValid();
    }

    sniffer_intf_t interf;
    uint32_t filter{ WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA };

    sys::FreeRtosQueuePod<sniffer_packet_info_t> m_queue{ CONFIG_WIFI_SERVICE_SNIFFER_QUEUE_LEN };

    static sniffer_runtime_t* g_instance;
};

sniffer_runtime_t* sniffer_runtime_t::g_instance{ nullptr };

#pragma pack( push, 1 )

struct FrameControl {
    // Full description: https://en.wikipedia.org/wiki/802.11_Frame_Types
    uint16_t protocolVersion : 2;
    uint16_t type : 2;
    uint16_t subType : 4;
    uint16_t toDs : 1;
    uint16_t fromDs : 1;
    uint16_t moreFrag : 1;
    uint16_t retry : 1;
    uint16_t pwrMgmt : 1;
    uint16_t moreData : 1;
    uint16_t wep : 1;
    uint16_t order : 1;

    FrameControl() = delete;
    FrameControl( const FrameControl& ) = delete;

    inline bool isManagementFrame() const {
        return 0b00 == type;
    }

    inline bool isControlFrame() const {
        return 0b01 == type;
    }

    inline bool isDataFrame() const {
        return 0b10 == type;
    }

    inline bool isExtensionFrame() const {
        return 0b11 == type;
    }

    // Methods for managemet frame

    inline bool isBeacon() const {
        return 0b1000 == subType;
    }

    // Methods for data frame
    inline bool isIbss() const {
        return 0 == toDs && 0 == fromDs;
    }

    inline bool isToAccessPoint() const {
        return 1 == toDs && 0 == fromDs;
    }

    inline bool isFromAccessPoint() const {
        return 0 == toDs && 1 == fromDs;
    }

    inline bool isWds() const {
        return 1 == toDs && 1 == fromDs;
    }
};
struct MacHeader {
    MacHeader() = delete;
    MacHeader( const MacHeader& ) = delete;

    FrameControl frameControl;
    int16_t duration;
    BssidType Addr1;
    BssidType Addr2;
    BssidType Addr3;
    int16_t seqCtl;
    unsigned char body[];
};

struct BodyElement {
    BodyElement() = delete;
    BodyElement( const BodyElement& ) = delete;

    uint8_t id;
    uint8_t dataLength;
    unsigned char data[];

    inline bool isSsid() const {
        return 0 == id;
    }
};

struct MgmtBodyMandatory {
    MgmtBodyMandatory() = delete;
    MgmtBodyMandatory( const MgmtBodyMandatory& ) = delete;

    uint64_t timestamp;
    uint16_t beaconInterval;
    uint16_t capabilityInfo;
    BodyElement firstElem;
};
#pragma pack( pop )

template <typename... Args>
inline void putInfoToQueue( Args&&... args ) {
    if( !sniffer_runtime_t::g_instance ) {
        return;
    }

    sniffer_packet_info_t packetInfo( std::forward<Args>( args )... );

    if( !sniffer_runtime_t::g_instance->m_queue.send( packetInfo ) ) {
        // ESP_LOGW( SNIFFER_TAG, "wifi queue full" );
    }
}

void handleManagementFrame( const MacHeader& mgmtHeader, const unsigned int payloadLen, const int rssi ) {
    if( !mgmtHeader.frameControl.isBeacon() ) {
        return;
    }

    const auto mgmtBodyMandatory = reinterpret_cast<const MgmtBodyMandatory*>( mgmtHeader.body );
    const auto& firstElem = mgmtBodyMandatory->firstElem;
    if( !firstElem.isSsid() ) {
        return;
    }

    // Addr1: DestinationAddr, Addr2: SourceAddr, Addr3: BSSID
    putInfoToQueue( reinterpret_cast<const char*>( firstElem.data ),
                    firstElem.dataLength,
                    mgmtHeader.Addr2,
                    mgmtHeader.Addr3,
                    rssi );
}

void handleDataFrame( const MacHeader& dataHeader, const unsigned int payloadLen, const int rssi ) {
    if( dataHeader.frameControl.isIbss() ) {
        // Addr1: DestinationAddr, Addr2: SourceAddr, Addr3: BSSID
        // Not used
    }
    else if( dataHeader.frameControl.isFromAccessPoint() ) {
        // Addr1: DestinationAddr, Addr2: BSSID, Addr3: SourceAddr
        putInfoToQueue( dataHeader.Addr3, dataHeader.Addr2, rssi );
    }
    else if( dataHeader.frameControl.isToAccessPoint() ) {
        // Addr1: BSSID, Addr2: SourceAddr, Addr3: DestinationAddr
        putInfoToQueue( dataHeader.Addr2, dataHeader.Addr1, rssi );
    }
    else if( dataHeader.frameControl.isWds() ) {
        // Addr1: RAddr, Addr2: TAddr, Addr3: DestinationAddr, Addr4: SourceAddr
        // Not used
    }
}

void wifi_sniffer_cb( const wifi_promiscuous_pkt_t* packet, const wifi_promiscuous_pkt_type_t type ) {
    if( 0 != packet->rx_ctrl.rx_state ) {
        // Packet received with errors
        return;
    }

    constexpr auto SNIFFER_PAYLOAD_FCS_LEN{ 4 };  // FCS is the 32bit checksum at the end if packet
    const auto payloadLen = packet->rx_ctrl.sig_len - SNIFFER_PAYLOAD_FCS_LEN;
    const auto header = reinterpret_cast<const MacHeader*>( packet->payload );

    switch( type ) {
    case WIFI_PKT_MGMT:
        // Don't waste resources to handle management packets because
        // I think that they are send only by access points
        // handleManagementFrame( *header, payloadLen, packet->rx_ctrl.rssi );
        break;
    case WIFI_PKT_CTRL:
        break;
    case WIFI_PKT_DATA:
        handleDataFrame( *header, payloadLen, packet->rx_ctrl.rssi );
        break;
    case WIFI_PKT_MISC:
        break;
    }
}

WifiSniffer::WifiSniffer( const sniffer_intf_t intf )
: m_ctx{ std::make_unique<sniffer_runtime_t>( intf ) } {}

WifiSniffer::~WifiSniffer() {
    if( m_isStarted ) {
        stop();
    }
}

bool WifiSniffer::initializeImpl() {
    return ParentType::initializeImpl() && bus::Bus::get().join( BUS_NAME, *this );
}

bool WifiSniffer::start() {
    if( m_isStarted ) {
        return true;
    }

    m_isStarted =
        isConnectedToAcessPoint() && m_ctx && m_ctx->isValid() && m_ctx->startWiFiPromicuousMode( wifi_sniffer_cb );

    return m_isStarted;
}

bool WifiSniffer::stop() {
    if( !m_isStarted ) {
        ESP_LOGW( SNIFFER_TAG, "Sniffer is already stopped" );
        return true;
    }

    m_isStarted = false;
    m_sendTp = ClockType::now();
    return m_ctx->stopWiFiPromicuousMode();
}

WifiSniffer::ClbkRet WifiSniffer::updateComponent( UpdContext& ctx ) {
    if( !m_isStarted && !start() ) {
        return setDelay( 10s );
    }

    m_updateTp = ClockType::now();
    constexpr auto RECV_PACKET_TIMEOUT_MS = 100ms;
    auto packetInfo = m_ctx->m_queue.recv( RECV_PACKET_TIMEOUT_MS );
    while( packetInfo ) {
        ctx.tryYield();

        auto [itr, isInserted] =
            m_beacons.try_emplace( packetInfo->m_addr, packetInfo->m_bssid, packetInfo->m_rssi, packetInfo->m_ssid );
        if( !isInserted && std::get<1>( itr->second ) < packetInfo->m_rssi ) {
            std::get<1>( itr->second ) = packetInfo->m_rssi;
        }

        auto newTp = ClockType::now();
        if( newTp - m_updateTp > 100ms ) {
            m_updateTp = newTp;
            return ClbkRet::Continue;
        }

        if( newTp - m_sendTp > 1s ) {
            m_sendTp = newTp;

            // Don't use this log in production code! For debug only!
            /*ESP_LOGW( SNIFFER_TAG, "-------> discovered wifi devices=%u", m_beacons.size() );
            for( const auto& bn : m_beacons ) {
                ESP_LOGI( SNIFFER_TAG,
                            "------------------> addr(%s), bssid(%s), rssi(%d), name(%s)",
                            bn.first.toHexString().c_str(),
                            std::get<0>( bn.second ).toHexString().c_str(),
                            std::get<1>( bn.second ).value(),
                            std::get<2>( bn.second ).data() );
            }*/
            send<responce::DiscoveredWifiBeacons>( responce::BUS_NAME, std::move( m_beacons ) );
        }

        packetInfo = m_ctx->m_queue.recv( RECV_PACKET_TIMEOUT_MS );
    }

    return ClbkRet::Continue;
};

}  // namespace wifi_srv2
}  // namespace network

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
