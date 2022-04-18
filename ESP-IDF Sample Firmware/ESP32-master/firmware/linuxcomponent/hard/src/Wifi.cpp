#include <assert.h>
#include "Wifi.h"

namespace hard {

Wifi& Wifi::get() {
    static Wifi wifi;
    return wifi;
}

bool Wifi::init() {
    return true;
}

bool Wifi::getMac( std::uint8_t dst[6] ) const {
    return true;
}

bool Wifi::getIpAddr( char* str, std::size_t maxlen ) const {
    return true;
}

bool Wifi::startAp( const char* ssid, AuthenticateMode authenticateMode, std::uint8_t maxClientCount ) const {
    return true;
}

bool Wifi::scanStart( std::uint32_t scanTimeout ) const {
    return true;
}

bool Wifi::setMode( Mode mode ) const {
    return true;
}

bool Wifi::setConfig( Mode mode, const Config& config ) const {
    return true;
}

bool Wifi::getConfig( Mode mode, Config* config ) const {
    assert( config != nullptr );
    return true;
}

bool Wifi::connect() {
    return true;
}

bool Wifi::disconnect() const {
    return true;
}

bool Wifi::restart() const {
    return true;
}

bool Wifi::initEventLoop( EventCallback cb, void* arg ) {
    return true;
}

Wifi::DisconnectReason Wifi::getDisconnectReason() const {
    return m_eventData.disconnectReason;
}

}  // namespace hard
