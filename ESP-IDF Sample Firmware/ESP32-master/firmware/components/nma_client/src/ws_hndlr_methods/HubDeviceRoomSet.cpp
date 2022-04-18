#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "lua/BusCommandLuaEvents.hpp"

#include "DeviceInfo.h"
#include "values/StringValue.h"
#include "lua/responce/UpdateDevice.h"
#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"
#include "ObjectSerializer.h"

#include "DebugUtils.h"

namespace nma {

using namespace common::resources;

using BlobType = storage::KeyValueStorage::BlobType;

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceRoomSet( const gason::JsonValue& json,
                                                                     common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    auto deviceId = params( tokens::SubId ).toString();
    auto roomId = params( tokens::RoomId ).toString();
    auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !devices || !deviceId || !roomId || !devices ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    BlobType buff;
    if( !( devices->getBlob( deviceId, buff ) ) ) {
        FUNC_LOGE( "Failed to get a current value for the key: %s", deviceId );
        return std::make_tuple( errors::ApplicationError, StringType() );
    }

    gateways::BaseDeviceInfo currentDeviceInfo;
    gateways::ObjectSerializer serializer;
    if( !serializer.deserialize( deviceId, std::move( buff ), currentDeviceInfo ) ) {
        FUNC_LOGE( "Failed to deserialize the key: %s", deviceId );
        return std::make_tuple( errors::ApplicationError, StringType() );
    }

    if( !currentDeviceInfo.m_parentDeviceId.empty() ) {
        return std::make_tuple( errors::InvalidRequest, StringType() );
    }

    std::set<std::string> nestedDevices, updatedDevices;
    nestedDevices.insert( deviceId );

    const auto end = devices->cend();
    for( auto itr = devices->cbegin(); itr != end; ++itr ) {
        auto id_std_str = *itr;
        std::string id( id_std_str.begin(), id_std_str.end() );

        if( updatedDevices.end() != updatedDevices.find( id ) ) {
            continue;
        }
        updatedDevices.emplace( id );

        auto data = itr.value();
        if( !data.is_type<BlobType>() ) {
            FUNC_LOGE( "Failed to get a current value for the key: %s", id.c_str() );
            // TODO: use more understandable for example 'not found'
            return std::make_tuple( errors::InvalidParams, StringType() );
        }
        auto& buff = data.get<BlobType>();
        gateways::BaseDeviceInfo deviceInfo;
        gateways::ObjectSerializer serializer;
        if( !serializer.deserialize( id_std_str, std::move( buff ), deviceInfo ) ) {
            FUNC_LOGE( "Failed to deserialize the key: %s", id.c_str() );
            // TODO: use more understandable
            return std::make_tuple( errors::InvalidParams, StringType() );
        }

        if( nestedDevices.find( id ) == nestedDevices.end() &&
            nestedDevices.find( deviceInfo.getParentDeviceId() ) == nestedDevices.end() ) {
            continue;
        }

        bool isRoomIdNeedToUpdate = false;

        const auto currentRoomId = deviceInfo.getRoomId();

        FUNC_LOGE( "if (currentRoomId && roomId): %s , %s", currentRoomId, roomId );
        if( currentRoomId && roomId ) {
            isRoomIdNeedToUpdate = strcmp( currentRoomId, roomId );
        }

        if( isRoomIdNeedToUpdate ) {
            deviceInfo.m_roomId = std::string( roomId );
            buff = serializer.serialize( deviceInfo );
            if( buff.size() == 0 ) {
                FUNC_LOGE( "Failed to serialize the key: %s", id.c_str() );
                return std::make_tuple( errors::ApplicationError, StringType() );
            }

            if( !devices->setBlob( id_std_str, buff ) ) {
                FUNC_LOGE( "Failed to write the key: %s", id.c_str() );
                // TODO: use more understandable
                return std::make_tuple( errors::InvalidParams, StringType() );
            }

            auto value = std::make_unique<common::StringValue>( roomId );
            auto cmd = std::make_unique<lua::responce::UpdateDevice>();
            if( !cmd ) {
                // TODO: use more understandable
                return std::make_tuple( errors::InvalidParams, StringType() );
            }

            cmd->m_deviceId = std::move( id_std_str );
            cmd->m_key.assign( common::resources::tokens::RoomId );
            cmd->m_value = std::move( value );
            m_client.send( lua::responce::BUS_NAME, std::move( cmd ) );

            m_client.send(::bus::lua_event::BUS_NAME,
                          ::bus::lua_event::deviceUpdated( deviceId, ::common::resources::tokens::RoomId, roomId ) );

            nestedDevices.emplace( std::move( id ) );
        }
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma
