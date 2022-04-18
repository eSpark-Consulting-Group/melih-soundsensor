#include "HubIDDataVer1.h"
#include <cstring>
#include <cassert>

namespace core {

HubIDDataVer1::HubIDDataVer1( const char* buff )
: m_buff( buff ) {
    assert( m_buff );
}

std::uint64_t HubIDDataVer1::id() const {
    std::uint64_t id = 0;
    std::memcpy( &id, m_buff + ID_OFFSET, ID_SIZE );
    return id;
}

const char* HubIDDataVer1::uuid() const {
    return m_buff + UUID_OFFSET;
}

const char* HubIDDataVer1::zwaveRegion() const {
    return m_buff + ZWAVE_REGION_OFFSET;
}

const char* HubIDDataVer1::defaultWiFiSSID() const {
    return m_buff + DEFAULT_WIFI_SSID_OFFSET;
}

const char* HubIDDataVer1::defaultWiFiPass() const {
    return m_buff + DEFAULT_WIFI_PASS_OFFSET;
}

const char* HubIDDataVer1::name() const {
    return nullptr;
}

}  // namespace core
