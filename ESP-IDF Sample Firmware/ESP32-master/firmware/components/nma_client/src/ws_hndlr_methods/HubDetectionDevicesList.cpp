// Developed according with
// https://confluence.mios.com/pages/viewpage.action?spaceKey=ELFD&title=Devices+Detection#DevicesDetection-hub.detection.devices.list

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"

#include "dev_dis/DevDiscovery.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDetectionDevicesList( const gason::JsonValue& json,
                                                                            common::JsonStreamGenerator& gen ) {
    if( !gen.writeOpeningObject() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    auto params = json( tokens::Params );
    if( !params || !params.isObject() ) {
        gen.writeClosingSymbol();
        return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
    }

    std::set<dev_dis::Uuid> uuids;
    if( auto ids = params( tokens::SubIdMulti ) ) {
        if( !ids.isArray() ) {
            gen.writeClosingSymbol();
            return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
        }

        for( const auto item : ids ) {
            auto uuid = dev_dis::Uuid::createFromHexStr( item.value.toString() );
            if( !uuid ) {
                gen.writeClosingSymbol();
                return std::make_tuple( errors::InvalidParams, errors::data::WrongParams );
            }

            uuids.insert( *uuid );
        }
    }

    auto instance = dev_dis::DevDiscovery::get();
    if( !gen.writePropertyKey( tokens::Devices ) || !gen.writeOpeningArray() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    const auto infoStateList = instance->getInfoStateList();
    for( const auto& [info, state] : infoStateList ) {
        bool isSuccess = gen.writeOpeningObject();
        isSuccess = isSuccess && gen.writeProperty( tokens::Id, info.getId().toHexString() );
        isSuccess = isSuccess && gen.writeProperty( tokens::Name, info.getName() );
        isSuccess = isSuccess && gen.writeProperty( tokens::Mac, info.getMac().toHexString() );
        isSuccess = isSuccess && gen.writeProperty( tokens::Protocol, tokens::Wifi );
        isSuccess = isSuccess && gen.writeProperty( tokens::Precision, info.getPrecision() );
        if( isSuccess && state ) {
            const auto& [rssi, timepoint] = *state;
            isSuccess = gen.writeProperty( tokens::Rssi, rssi );
            isSuccess = isSuccess && gen.writeProperty( tokens::DetectedLastTime, dev_dis::toDateString( timepoint ) );
        }
        isSuccess = isSuccess && gen.writeClosingSymbol();

        if( !isSuccess ) {
            return std::make_tuple( errors::GenJsonFailed, StringType{} );
        }
    }

    if( !gen.writeClosingSymbol() || !gen.writeClosingSymbol() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
