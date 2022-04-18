#pragma once

#include <cstdint>

namespace network {

namespace mdns {

constexpr const char* hostname = "atom32";
constexpr const char* defaultInstanceName = "Atom32";
constexpr const char* serviceType = "_ezlo";
constexpr const char* serviceProtocol = "_tcp";
// constexpr uint16_t servicePort = 17000;

constexpr const char* serialParam = "Serial";
constexpr const char* firmwareVersionParam = "Firmware Version";
constexpr const char* vendorParam = "Vendor";
constexpr const char* hubTypeParam = "Hub Type";

bool start();
bool setInstanceName( const char* name );

void setUpService();
bool fillInstanceNameFromEzloNodeName();
bool setSerial( const char* serial );
bool setFirmwareVersion( const char* firmwareVersion );
bool setVendor( const char* vendorName );
bool setHubType( const char* hubType );

};  // namespace mdns

}  // namespace network
