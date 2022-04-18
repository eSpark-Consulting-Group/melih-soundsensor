#include "NmaJsonHandler.h"

#if defined( CONFIG_CLOUD_DEVICES ) and !defined( CONFIG_UART_API )

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "ColudDevicesManager.h"
#include "DebugUtils.h"

#include <map>

namespace cloud_client {

using namespace common::resources;

NmaJsonHandler::ResponceType NmaJsonHandler::onCloudAbstractsUpdated( const gason::JsonValue& json,
                                                                      common::JsonStreamGenerator& gen ) {
    auto devices = json( tokens::Result )( tokens::Devices );
    for( auto device_node : devices ) {
        auto fieldsNode = device_node.value;
        auto uuid = device_node.key;

        std::map<std::string, std::string> fields;
        for( auto field : fieldsNode ) {
            auto name = field.key;
            auto val = field.value;

            std::string key;
            std::string value;
            key.assign( name );
            value.assign( val.toString() );

            fields.insert( std::pair<std::string, std::string>( key, value ) );
        }

        std::string id{ uuid };
        gateways::cloud::ColudDevicesManager::get().deviceUpdate( id, fields );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace cloud_client

#endif  // defined(CONFIG_CLOUD_DEVICES) and !defined(CONFIG_UART_API)
