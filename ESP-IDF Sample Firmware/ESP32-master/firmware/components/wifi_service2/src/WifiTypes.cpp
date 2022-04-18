#include "wifi_service2/WifiTypes.h"

extern "C" {
#include <sys/socket.h>
}  // extern "C"

#include <cstring>

namespace network {
namespace wifi_srv2 {

namespace {

std::string arrayToHexString( const std::array<uint8_t, 6>& arr ) {
    // bufSize = arr.size() numbers of 2 char + (arr.size() - 1) of ":" delimiters + 1 of '\0'
    constexpr auto bufSize = 2 * arr.size() + ( arr.size() - 1 ) + 1;
    char buffer[bufSize];
    const char fmt[] = "%02X:%02X:%02X:%02X:%02X:%02X";
    std::snprintf( buffer, bufSize, fmt, arr.at( 0 ), arr.at( 1 ), arr.at( 2 ), arr.at( 3 ), arr.at( 4 ), arr.at( 5 ) );
    return buffer;
}

}  // namespace

int Rssi::rssiToPercent() const {
    if( m_value > -40 ) {
        return 100;
    }
    else {
        if( m_value < -79 ) {
            return 1;
        }
        else {
            return 100 + 2.5 * ( 40 + m_value );
        }
    }
}

std::array<uint8_t, 6> WifiApInfo::toArray() const {
    std::array<uint8_t, 6> data;
    std::memcpy( data.data(), bssid, data.size() );
    return std::move( data );
}

std::optional<MacType> MacType::get() {
    MacType mac;
    if( ESP_OK == esp_wifi_get_mac( WIFI_IF_STA, mac.data() ) ||
        ESP_OK == esp_read_mac( mac.data(), ESP_MAC_WIFI_STA ) ) {
        return std::make_optional<MacType>( mac );
    }

    return std::nullopt;
}

IPAddress IpInfo::networkID() const {
    return calculateNetworkID( getGateway(), getNetmask() );
}

uint8_t IpInfo::subnetCIDR() const {
    return calculateSubnetCIDR( getNetmask() );
}

std::string IpInfo::ipToStr() const {
    constexpr auto bufSize = 16;
    char buff[bufSize];
    inet_ntop( AF_INET, &ip, buff, bufSize );
    return buff;
}

std::string IpInfo::netmaskToStr() const {
    constexpr auto bufSize = 16;
    char buff[bufSize];
    inet_ntop( AF_INET, &netmask, buff, bufSize );
    return buff;
}

std::optional<WifiApInfo> WifiApInfo::get() {
    WifiApInfo apInfo = {};
    return ESP_OK == esp_wifi_sta_get_ap_info( &apInfo ) ? std::make_optional<WifiApInfo>( apInfo ) : std::nullopt;
}

std::string WifiApInfo::authModeString() const {
    switch( authmode ) {
    case WIFI_AUTH_OPEN:
        return "none";
    case WIFI_AUTH_WEP:
        return "wep";
    case WIFI_AUTH_WPA_PSK:
        return "psk";
    case WIFI_AUTH_WPA2_PSK:
    case WIFI_AUTH_WPA_WPA2_PSK:
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "psk2";
    default:
        return "unknow";
    }
}

std::string WifiApInfo::bssidToHexString() const {
    const auto& bs = reinterpret_cast<const MacType&>( bssid );
    return bs.toHexString();
}

std::optional<BssidType> BssidType::fromString( std::string bsstr ) {
    // bsstr has format "XX:XX:XX:XX:XX:XX" where XX is two char hexadecimal number
    if( 17 != bsstr.size() ) {
        return std::nullopt;
    }

    network::wifi_srv2::BssidType bs;
    int val;
    for( std::size_t i = 0; i < bs.size() - 1; ++i ) {
        std::size_t pos{ 0 };
        val = std::stoi( bsstr, &pos, 16 );
        bs[i] = val;
        if( 2 == pos && ':' == bsstr[pos] ) {
            bsstr.erase( 0, pos + 1 );
        }
        else {
            return std::nullopt;
        }
    }
    std::size_t pos{ 0 };
    val = std::stoi( bsstr, &pos, 16 );
    if( 2 != pos ) {
        return std::nullopt;
    }
    bs[bs.size() - 1] = val;

    return std::make_optional( bs );
}

std::string BssidType::toHexString() const {
    return arrayToHexString( *this );
}

}  // namespace wifi_srv2
}  // namespace network
