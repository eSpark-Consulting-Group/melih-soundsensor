#include "wifi_service2/IPAddress.h"
#include <sys/socket.h>

namespace network {
namespace wifi_srv2 {

std::string Ip4Addr::toString() const {
    constexpr auto bufSize = 16;
    char buff[bufSize];
    inet_ntop( AF_INET, this, buff, bufSize );
    return buff;
}

IPAddress calculateNetworkID( IPAddress ip, IPAddress subnet ) {
    IPAddress networkID;

    for( size_t i = 0; i < 4; i++ )
        networkID[i] = subnet[i] & ip[i];

    return networkID;
}

IPAddress calculateBroadcast( IPAddress ip, IPAddress subnet ) {
    IPAddress broadcastIp;

    for( int i = 0; i < 4; i++ )
        broadcastIp[i] = ~subnet[i] | ip[i];

    return broadcastIp;
}

uint8_t calculateSubnetCIDR( IPAddress subnetMask ) {
    uint8_t CIDR = 0;

    for( uint8_t i = 0; i < 4; i++ ) {
        if( subnetMask[i] == 0x80 )  // 128
            CIDR += 1;
        else if( subnetMask[i] == 0xC0 )  // 192
            CIDR += 2;
        else if( subnetMask[i] == 0xE0 )  // 224
            CIDR += 3;
        else if( subnetMask[i] == 0xF0 )  // 242
            CIDR += 4;
        else if( subnetMask[i] == 0xF8 )  // 248
            CIDR += 5;
        else if( subnetMask[i] == 0xFC )  // 252
            CIDR += 6;
        else if( subnetMask[i] == 0xFE )  // 254
            CIDR += 7;
        else if( subnetMask[i] == 0xFF )  // 255
            CIDR += 8;
    }

    return CIDR;
}

}  // namespace wifi_srv2
}  // namespace network