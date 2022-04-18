#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "DebugUtils.h"
#include "HTTPRequest.h"
#include "Utils.h"

#include "IValue.h"
#include "IValueCast.h"
#include "values/ArrayValue.h"
#include "values/TableValue.h"
#include "values/StringValue.h"

#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"
#include "ObjectSerializer.h"

#include "BusCommandHubDeviceFirmwareUpdate.h"
#include "ZWaveZNet.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceFirmwareUpdate( const gason::JsonValue& json,
                                                                            common::JsonStreamGenerator& jsGen ) {
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !devices ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    auto jsUpdates = ::common::makeIval( json( tokens::Params )( tokens::Updates ) );
    if( jsUpdates ) {
        auto updates = json( tokens::Params )( tokens::Updates );

        for( auto update : updates ) {
            auto devicesArr = update.value( tokens::Devices );
            if( devicesArr.isArray() ) {
                for( auto device : devicesArr ) {
                    if( !device.value.isString() ) {
                        return std::make_tuple( errors::InvalidParams, StringType() );
                    }

                    bool isValidDeviceId = false;
                    const auto deviceId = device.value.toString();
                    gateways::enumerateDevicesFromDb( [&]( const gateways::BaseDeviceInfo& deviceInfo ) {
                        if( !strcmp( deviceInfo.getId(), deviceId ) ) {
                            auto parrentId = deviceInfo.getParentDeviceId();
                            isValidDeviceId = strlen( parrentId ) == 0;
                        }
                        return true;
                    } );

                    if( !isValidDeviceId ) {
                        return std::make_tuple( errors::ApplicationError, "Devices not found!" );
                    }
                }
            }
            else {
                return std::make_tuple( errors::ApplicationError, "Devices not found!" );
            }

            auto firmwares = update.value( tokens::Firmware );

            if( firmwares.isArray() ) {
                for( auto firmware : firmwares ) {
                    if( firmware.value.isObject() ) {
                        auto value = firmware.value;

                        if( !( value( tokens::Id ) && ( value( tokens::Image ) ) && value( tokens::Checksum ) ) ) {
                            return std::make_tuple( errors::InvalidParams, StringType() );
                        }
                    }
                }
            }
            else {
                return std::make_tuple( errors::InvalidParams, StringType() );
            }
        }

        auto arrayUpdates = common::iValueCast<common::ArrayValue>( jsUpdates.get() );
        for( auto& field : *arrayUpdates ) {
            auto table = common::iValueCast<common::TableValue>( field.get() );
            auto arrayDevices =
                common::iValueCast<common::ArrayValue>( ( *table )[common::resources::tokens::Devices] );
            for( auto& deviceId : *arrayDevices ) {
                auto devId = common::iValueCast<common::StringValue>( deviceId.get() )->get();
                std::vector<char> buff;
                if( !( devices && devices->getBlob( devId, buff ) ) ) {
                    FUNC_LOGE( "Failed to get device whith id: %s", devId.c_str() );
                    return std::make_tuple( errors::InvalidParams, std::string() );
                }
            }
        }
        auto cmd = std::make_unique<bus::command::HubDeviceFirmwareUpdate>( std::move( jsUpdates ) );
        m_client.send( "debug", std::move( cmd ) );
    }

    return std::make_tuple( errors::NotError, std::string() );
}

}  // namespace nma
