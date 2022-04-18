#include "HubIDDataVer2.h"
#include <cassert>
#include <cstring>
#include <FlashUtils.h>

namespace core {

HubIDDataVer2::HubIDDataVer2( const char* buff, std::size_t flash_offset )
: HubIDDataVer1( buff )
, m_flash_offset( flash_offset ) {
    assert( m_buff );
}

const char* HubIDDataVer2::name() const {
    return m_buff + NAME_OFFSET;
}

bool HubIDDataVer2::setId( std::uint64_t id ) {
    char buff[ID_SIZE];
    std::memset( buff, 0, sizeof( buff ) );
    std::memcpy( buff, &id, std::min( sizeof( id ), sizeof( buff ) ) );
    return hard::writeToFlash( m_flash_offset + ID_OFFSET, buff, ID_SIZE );
}

bool HubIDDataVer2::setUUID( const char* data, const size_t size ) {
    if( size > UUID_SIZE ) {
        return false;
    }
    return hard::writeToFlash( m_flash_offset + UUID_OFFSET, data, size );
}

bool HubIDDataVer2::setZwaveRegion( const std::string& zwaveRegion ) {
    if( zwaveRegion.size() > ZWAVE_REGION_SIZE ) {
        return false;
    }
    return hard::writeToFlash( m_flash_offset + ZWAVE_REGION_OFFSET, zwaveRegion.c_str(), zwaveRegion.size() + 1 );
}

bool HubIDDataVer2::setDefaultWiFiSSID( const std::string& defaultWiFiSSID ) {
    if( defaultWiFiSSID.size() > DEFAULT_WIFI_SSID_MAX_SIZE ) {
        return false;
    }
    return hard::writeToFlash(
        m_flash_offset + DEFAULT_WIFI_SSID_OFFSET, defaultWiFiSSID.c_str(), defaultWiFiSSID.size() + 1 );
}

bool HubIDDataVer2::setDefaultWiFiPass( const std::string& defaultWiFiPass ) {
    if( defaultWiFiPass.size() > DEFAULT_WIFI_PASS_MAX_SIZE ) {
        return false;
    }
    return hard::writeToFlash(
        m_flash_offset + DEFAULT_WIFI_PASS_OFFSET, defaultWiFiPass.c_str(), defaultWiFiPass.size() + 1 );
}

bool HubIDDataVer2::setProductName( const std::string& product_name ) {
    if( product_name.size() > NAME_SIZE ) {
        return false;
    }

    if( product_name.size() > 0 ) {
        return hard::writeToFlash( m_flash_offset + NAME_OFFSET, product_name.c_str(), product_name.size() + 1 );
    }
    else {
        return hard::writeToFlash( m_flash_offset + NAME_OFFSET, "", 1 );
    }
}

}  // namespace core
