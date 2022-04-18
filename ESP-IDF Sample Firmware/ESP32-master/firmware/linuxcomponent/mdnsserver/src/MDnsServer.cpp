#include "MDnsServer.h"

namespace network {

bool mdns::start() {
    return true;
}

bool mdns::setInstanceName( const char* name ) {
    return true;
}

void mdns::setUpService() {}

bool mdns::fillInstanceNameFromEzloNodeName() {}

bool mdns::setSerial( const char* serial ) {
    return true;
}

bool mdns::setFirmwareVersion( const char* firmwareVersion ) {
    return true;
}

bool mdns::setVendor( const char* vendorName ) {
    return true;
}

bool mdns::setHubType( const char* hubType ) {
    return true;
}

}  // namespace network
