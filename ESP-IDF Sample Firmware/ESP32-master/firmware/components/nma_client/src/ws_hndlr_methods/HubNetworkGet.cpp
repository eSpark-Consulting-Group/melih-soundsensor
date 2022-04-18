#include "sdkconfig.h"

#ifndef CONFIG_UART_API

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "wifi_service2/Common.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubNetworkGet( const gason::JsonValue& json,
                                                                  common::JsonStreamGenerator& jsGen ) {
    bool result = jsGen.writeOpeningObject();
    {
        result = result && jsGen.writePropertyKey( tokens::Interfaces );
        result = result && jsGen.writeOpeningArray();
        {
            result = result && jsGen.writeOpeningObject();
            {
                result = result && jsGen.writeProperty( "_id", wifi::tokens::WLAN );

                auto mac = network::wifi_srv2::MacType::get();
                if( result && mac ) {
                    result = jsGen.writeProperty( tokens::Hwaddr, mac->toHexString() );
                }

                result = result && jsGen.writePropertyKey( tokens::Ipv4 );
                result = result && jsGen.writeOpeningObject();
                if( result ) {
                    const auto ipInfo = network::wifi_srv2::getStaIpInfo();
                    result = result && ipInfo && jsGen.writeProperty( tokens::Ip, ipInfo->getAddr().toString() );
                    result = result && ipInfo && jsGen.writeProperty( tokens::Mask, ipInfo->getNetmask().toString() );
                    result = result && jsGen.writeProperty( tokens::Mode, "dhcp" );
                    result = result && jsGen.writeClosingSymbol();
                }

                result = result && jsGen.writeProperty( tokens::Status, "up" );
                result = result && jsGen.writeProperty( tokens::Type, "wifi" );
                result = result && jsGen.writeProperty( tokens::Enabled, "auto" );

                result = result && jsGen.writePropertyKey( tokens::SignalQuality );
                result = result && jsGen.writeOpeningObject();
                if( result ) {
                    const auto apInfo = network::wifi_srv2::WifiApInfo::get();
                    if( apInfo ) {
                        result = result && jsGen.writeProperty( "rssi", apInfo->getRssi().m_value );
                        result = result && jsGen.writeProperty( "level", apInfo->getRssi().rssiToPercent() );
                    }
                    result = result && jsGen.writeClosingSymbol();
                }

                result = result && jsGen.writeProperty( tokens::InternetAvailable, true );
                result = result && jsGen.writePropertyKey( "wifi" );
                result = result && jsGen.writeOpeningObject();
                auto wifiApInfo = network::wifi_srv2::getStaApInfo();
                if( wifiApInfo && result ) {
                    result = jsGen.writeProperty( "channel", wifiApInfo->primary );
                    result = result && jsGen.writeProperty( tokens::Mode, "sta" );
                    result = result && jsGen.writePropertyKey( tokens::Network );
                    result = result && jsGen.writeOpeningObject();
                    {
                        result = result && jsGen.writeProperty( "encryption", "psk2" );
                        result = result && jsGen.writeProperty( tokens::Ssid, wifiApInfo->ssidToString() );
                        auto staConf = network::wifi_srv2::getStaConfig();
                        if( staConf && result ) {
                            result = jsGen.writeProperty( tokens::Key, staConf->passwordToString() );
                        }
                    }
                    result = result && jsGen.writeClosingSymbol();
                    result = result && jsGen.writeProperty( tokens::Region, "00" );
                }
                result = result && jsGen.writeClosingSymbol();
            }
            result = result && jsGen.writeClosingSymbol();
        }
        result = result && jsGen.writeClosingSymbol();
    }
    result = result && jsGen.writeClosingSymbol();

    return result ? std::make_tuple( errors::NotError, StringType() ) :
                    std::make_tuple( errors::GenJsonFailed, StringType{} );
}

}  // namespace nma

#endif  // not CONFIG_UART_API
