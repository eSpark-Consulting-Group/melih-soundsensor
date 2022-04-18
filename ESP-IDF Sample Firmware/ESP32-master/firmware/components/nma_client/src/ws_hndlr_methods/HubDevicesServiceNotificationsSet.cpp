#include "NmaWebSocketHandler.h"

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"
#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"
#include "DeviceInfo.h"
#include "ObjectSerializer.h"

#include <tuple>

namespace nma {

using namespace common::resources;

using BlobType = storage::KeyValueStorage::BlobType;

WebSocketHandler::ResponceType WebSocketHandler::onHubDevicesServiceNotificationsSet(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& gen ) {
    if( !gen.writeEmptyObject() || !gen.writeItemDelimiter() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    auto jsItemId = json( tokens::Params )( tokens::SubIdMulti );
    if( !jsItemId.isArray() ) {
        return std::make_tuple( errors::InvalidParams, errors::data::RpcParamsWrongField );
    }

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !stor ) {
        return std::make_tuple( errors::ApplicationError, errors::data::NoDeviceFound );
    }

    std::set<std::string> devIds;
    for( const auto item : jsItemId ) {
        const char* cstr = item.value.toString();
        if( cstr ) {
            std::string key( cstr );
            if( !stor->exists( key ) ) {
                return std::make_tuple( errors::ApplicationError, errors::data::NoDeviceFound );
            }
            devIds.insert( std::move( key ) );
        }
    }

    std::set<std::string> processed;
    auto it = stor->cbegin(), end = stor->cend();
    for( ; end != it; ++it ) {
        if( processed.find( *it ) != processed.end() ) {
            continue;
        }

        gateways::BaseDeviceInfo deviceInfo;
        gateways::ObjectSerializer serializer;
        auto val = it.value();
        if( !val.is_type<BlobType>() || val.get<BlobType>().empty() ||
            !serializer.deserialize( *it, std::move( val.get<BlobType>() ), deviceInfo ) ) {
            return std::make_tuple( errors::InternalError, errors::data::EzloInternaleError );
        }

        const bool isServiceNotification = devIds.find( *it ) != devIds.end();
        deviceInfo.setServiceNotification( isServiceNotification );

        auto buff = serializer.serialize( deviceInfo );
        if( 0 == buff.size() || !stor->setBlob( *it, buff ) ) {
            return std::make_tuple( errors::InternalError, errors::data::EzloInternaleError );
        }

        processed.emplace( *it );
    }

    return std::make_tuple( errors::NotError, std::string() );
}

}  // namespace nma
