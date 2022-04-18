// Developed according with
// https://confluence.mios.com/display/ELFD/Devices+Detection#DevicesDetection-hub.detection.device.set

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "dev_dis/DevDiscovery.h"
#include "NmaBroadcastsSender.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDetectionDeviceSet( const gason::JsonValue& json,
                                                                          common::JsonStreamGenerator& gen ) {
    if( !gen.writeOpeningObject() || !gen.writeClosingSymbol() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    const auto uuidCStr = params( tokens::SubId ).toString();
    const auto uuid = dev_dis::Uuid::createFromHexStr( uuidCStr );
    if( !uuid ) {
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    if( const auto protocolJs = params( tokens::Protocol ) ) {
        if( const auto protocolCStr = protocolJs.toString() ) {
            if( 0 != std::strcmp( protocolCStr, tokens::WifiDevice ) ) {
                return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
            }
        }
        else {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
    }

    std::optional<std::string> name;
    if( const auto nameJsStr = params( tokens::Name ) ) {
        const auto nameCStr = nameJsStr.toString();
        if( !nameCStr ) {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
        name = nameCStr;
    }

    std::optional<dev_dis::MacAddr> addr;
    if( const auto addrJsStr = params( tokens::Mac ) ) {
        if( !( addr = dev_dis::MacAddr::fromString( addrJsStr.toString() ) ) ) {
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
    }

    std::optional<dev_dis::Precision> precision;
    if( const auto precisionJs = params( tokens::Precision ) ) {
        bool isSuccess;
        precision = precisionJs.toNumber( &isSuccess );
        if( !isSuccess || precision < 1 || precision > 100 ) {
            return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
        }
    }

    auto instance = dev_dis::DevDiscovery::get();
    if( !instance || !instance->editDevice( *uuid, name, addr, precision ) ) {
        gen.writeClosingSymbol();
        return std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
