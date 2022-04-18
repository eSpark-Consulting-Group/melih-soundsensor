#include "MDnsServer.h"

#include <mdns.h>
#include <Ezlo.h>
#include "FirmwareVersion.h"
#include "DebugUtils.h"

namespace network {

bool mdns::start() {
    esp_err_t error = mdns_init();
    if( error != ESP_OK ) {
        FUNC_LOGE( "[mdns::start] Error occured while trying to initialize mdns: 0x%X", error );
        return false;
    }

    char name[32];
    sprintf( name, "HUB%s", core::serial() );
    mdns_hostname_set( name );
    mdns_instance_name_set( defaultInstanceName );

    error = mdns_service_add( nullptr, serviceType, serviceProtocol, core::Ezlo::getOfflineServerPort(), nullptr, 0 );
    if( error != ESP_OK ) {
        FUNC_LOGE( "[mdns::start] Error occured while trying to add %s service: 0x%X", serviceType, error );
        return false;
    }

    return true;
}

bool mdns::setInstanceName( const char* name ) {
    esp_err_t error = mdns_service_instance_name_set( serviceType, serviceProtocol, name );
    if( error != ESP_OK ) {
        FUNC_LOGE( "[mdns::setInstanceName] Error occured while setting instance name: 0x%X", error );
        return false;
    }
    return true;
}

void mdns::setUpService() {
    fillInstanceNameFromEzloNodeName();
    setSerial( core::serial() );
    setFirmwareVersion( FIRMWARE_VERSION );
    setVendor( "eZLO" );
    auto ezlo = core::Ezlo::get();
    if( ezlo ) {
        setHubType( ezlo->hubType() );
    }
}

bool mdns::fillInstanceNameFromEzloNodeName() {
    auto ezlo = core::Ezlo::get();
    if( !ezlo ) {
        return false;
    }

    char name[32];
    sprintf( name, "eZLO %s controller (%s)", ezlo->hubType(), core::serial() );
    return setInstanceName( name );
}

bool mdns::setSerial( const char* serial ) {
    esp_err_t error = mdns_service_txt_item_set( serviceType, serviceProtocol, serialParam, serial );
    if( error != ESP_OK ) {
        FUNC_LOGE( "[mdns::setSerial] Error occured while setting serial param for mDns service: 0x%X", error );
        return false;
    }
    return true;
}

bool mdns::setFirmwareVersion( const char* firmwareVersion ) {
    esp_err_t error = mdns_service_txt_item_set( serviceType, serviceProtocol, firmwareVersionParam, firmwareVersion );
    if( error != ESP_OK ) {
        FUNC_LOGE(
            "[mdns::setFirmwareVersion] Error occured while setting firmware version param for mDns service: 0x%X\n",
            error );
        return false;
    }
    return true;
}

bool mdns::setVendor( const char* vendorName ) {
    esp_err_t error = mdns_service_txt_item_set( serviceType, serviceProtocol, vendorParam, vendorName );
    if( error != ESP_OK ) {
        FUNC_LOGE( "[mdns::setVendor] Error occured while setting vendor name param for mDns service: 0x%X", error );
        return false;
    }
    return true;
}

bool mdns::setHubType( const char* hubType ) {
    esp_err_t error = mdns_service_txt_item_set( serviceType, serviceProtocol, hubTypeParam, hubType );
    if( error != ESP_OK ) {
        FUNC_LOGE( "[mdns::setHubType] Error occured while setting hub type param for mDns service: 0x%X", error );
        return false;
    }
    return true;
}

}  // namespace network
