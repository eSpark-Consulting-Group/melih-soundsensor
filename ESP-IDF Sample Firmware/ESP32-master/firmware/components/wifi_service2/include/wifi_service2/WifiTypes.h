#pragma once

#include "wifi_service2/IPAddress.h"
#include "IWriteStream.h"
#include "IReadStream.h"

#include <esp_wifi.h>

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <optional>
#include <cstring>

namespace network {
namespace wifi_srv2 {

struct BssidType : std::array<uint8_t, 6> {
    using ParentType = std::array<uint8_t, 6>;

    BssidType()
    : ParentType{ 0 } {}

    BssidType( const ParentType& bssid )
    : ParentType{ bssid } {}

    static std::optional<BssidType> fromString( std::string bsstr );

    static inline std::optional<BssidType> fromString( const char* bsstr ) {
        return bsstr ? fromString( std::string{ bsstr } ) : std::nullopt;
    }

    std::string toHexString() const;

    inline bool read( common::IReadStream& stream ) {
        return stream.read( static_cast<ParentType&>( *this ) );
    }

    inline bool write( common::IWriteStream& stream ) const {
        return stream.write( static_cast<const ParentType&>( *this ) );
    }
};

struct MacType : BssidType {
    static std::optional<MacType> get();
};

struct IpInfo : esp_netif_ip_info_t {
    inline IPAddress getAddr() const {
        return ip.addr;
    }

    inline IPAddress getNetmask() const {
        return netmask.addr;
    }

    inline IPAddress getGateway() const {
        return gw.addr;
    }

    IPAddress networkID() const;

    uint8_t subnetCIDR() const;

    std::string ipToStr() const;

    std::string netmaskToStr() const;
};

struct Rssi {
    Rssi()
    : m_value{ std::numeric_limits<int8_t>::min() } {}

    Rssi( const int value )
    : m_value{ value } {}

    int rssiToPercent() const;

    inline auto value() const {
        return m_value;
    }

    inline operator int() const {
        return m_value;
    }

    inline bool read( common::IReadStream& stream ) {
        return stream.read( m_value );
    }

    inline bool write( common::IWriteStream& stream ) const {
        return stream.write( m_value );
    }

    int m_value;
};

struct WifiApInfo : wifi_ap_record_t {
    static std::optional<WifiApInfo> get();

    std::string authModeString() const;

    inline std::string ssidToString() const {
        return reinterpret_cast<const char*>( ssid );
    }

    inline const char* ssidToCString() const {
        return reinterpret_cast<const char*>( ssid );
    }

    std::string bssidToHexString() const;

    inline Rssi getRssi() const {
        return rssi;
    }

    inline bool isOpen() const {
        return WIFI_AUTH_OPEN == authmode;
    }

    std::array<uint8_t, 6> toArray() const;

    inline bool operator<( const WifiApInfo& rv ) const {
        return rssi == rv.rssi ? 0 < std::strcmp( ssidToCString(), rv.ssidToCString() ) : rssi > rv.rssi;
    }

    inline bool operator==( const WifiApInfo& rv ) const {
        return 0 == memcmp( this, &rv, sizeof( network::wifi_srv2::WifiApInfo ) );
    }
};

using WifiApInfoVecType = std::vector<WifiApInfo>;
using WifiApInfoMapType = std::map<std::string, WifiApInfo>;
using WifiApInfoSetType = std::set<WifiApInfo>;

enum class HttpServerState { Inactive, Started, Run, Shutdowned };

enum class JoinResult {
    Success,
    AuthFailed,
    NoApFound,
    HandshakeTimeout,
    BeaconTimeout,
    AssociationFailed,
    NoInternetConnection,
    Undefined,
    Last
};

struct StaConfig : wifi_sta_config_t {
    StaConfig( wifi_sta_config_t conf )
    : wifi_sta_config_t{ conf } {}

    std::string ssidToString() const {
        return std::string{ reinterpret_cast<const char*>( ssid ) };
    }

    std::string passwordToString() const {
        return std::string{ reinterpret_cast<const char*>( password ) };
    }

    bool isValid() const {
        return !ssidToString().empty();
    }
};

}  // namespace wifi_srv2
}  // namespace network
