#pragma once

#include <esp_wifi.h>

#include <cstdint>
#include <string>

namespace network {
namespace wifi_srv2 {

class Ip4Addr : esp_ip4_addr_t {
public:
    enum class Type { IP, Netmask, GW };

    Ip4Addr()
    : esp_ip4_addr_t{}
    , m_type{ Type::IP } {}

    template <typename T>
    Ip4Addr( const T& _addr, Type _type = Type::IP )
    : esp_ip4_addr_t{ _addr }
    , m_type{ _type } {}

    Ip4Addr( uint8_t _first_octet,
             uint8_t _second_octet,
             uint8_t _third_octet,
             uint8_t _fourth_octet,
             Type _type = Type::IP ) {
        union {
            uint8_t bytes[4];
            uint32_t dword;
        };

        bytes[0] = _first_octet;
        bytes[1] = _second_octet;
        bytes[2] = _third_octet;
        bytes[3] = _fourth_octet;

        Ip4Addr( dword, _type );
    }

    operator uint32_t() const {
        return esp_ip4_addr_t::addr;
    }

    bool operator==( const Ip4Addr& _addr ) const {
        return esp_ip4_addr_t::addr == _addr.addr;
    }

    uint8_t operator[]( const int _index ) const {
        const uint8_t* bt = reinterpret_cast<uint8_t*>( esp_ip4_addr_t::addr );
        return bt[_index];
    }

    uint8_t& operator[]( const int _index ) {
        uint8_t* bt = reinterpret_cast<uint8_t*>( esp_ip4_addr_t::addr );
        return bt[_index];
    }

    std::string toString() const;

private:
    Type m_type;
};

using IPAddress = Ip4Addr;

IPAddress calculateNetworkID( IPAddress ip, IPAddress subnet );

IPAddress calculateBroadcast( IPAddress ip, IPAddress subnet );

uint8_t calculateSubnetCIDR( IPAddress subnetMask );

struct IPv6Address : esp_ip6_addr_t {
    IPv6Address()
    : esp_ip6_addr_t{} {}

    IPv6Address( const esp_ip6_addr_t& _ip6Addr )
    : esp_ip6_addr_t{ _ip6Addr } {}
};

}  // namespace wifi_srv2
}  // namespace network
