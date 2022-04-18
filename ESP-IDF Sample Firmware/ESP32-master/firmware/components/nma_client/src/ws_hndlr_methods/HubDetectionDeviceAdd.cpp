// Developed according with
// https://confluence.mios.com/pages/viewpage.action?spaceKey=ELFD&title=Devices+Detection#DevicesDetection-hub.detection.device.add

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "dev_dis/DevDiscovery.h"

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDetectionDeviceAdd( const gason::JsonValue& json,
                                                                          common::JsonStreamGenerator& gen ) {
    if( !gen.writeOpeningObject() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        gen.writeClosingSymbol();
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    if( const auto protocolJs = params( tokens::Protocol ) ) {
        if( const auto protocolCStr = protocolJs.toString() ) {
            if( 0 != std::strcmp( protocolCStr, tokens::WifiDevice ) ) {
                gen.writeClosingSymbol();
                return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
            }
        }
        else {
            gen.writeClosingSymbol();
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }
    }

    const auto nameCStr = params( tokens::Name ).toString();
    if( !nameCStr ) {
        gen.writeClosingSymbol();
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    const auto addrCStr = params( tokens::Mac ).toString();
    if( !addrCStr ) {
        gen.writeClosingSymbol();
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }
    auto addr = dev_dis::MacAddr::fromString( addrCStr );
    if( !addr ) {
        gen.writeClosingSymbol();
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    bool isSuccess;

    unsigned int precision;
    {
        const auto precisionJs = params( tokens::Precision );
        if( precisionJs ) {
            precision = precisionJs.toNumber( &isSuccess );
            if( !isSuccess ) {
                gen.writeClosingSymbol();
                return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
            }
            else if( precision < 1 || precision > 100 ) {
                gen.writeClosingSymbol();
                return std::make_tuple( errors::InvalidParams, errors::data::RangeInvalid );
            }
        }
        else {
            precision = 30;  // Set precision to default value
        }
    }

    auto instance = dev_dis::DevDiscovery::get();
    if( !instance ) {
        gen.writeClosingSymbol();
        return std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }

    const auto devUuid = instance->addDevice( nameCStr, *addr, precision );
    if( !devUuid ) {
        gen.writeClosingSymbol();
        return std::make_tuple( errors::InternalError, errors::data::InternalOperationError );
    }

    const auto devUuidStr = devUuid.toHexString();

    if( !gen.writeProperty( tokens::Id, devUuidStr ) || !gen.writeClosingSymbol() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
