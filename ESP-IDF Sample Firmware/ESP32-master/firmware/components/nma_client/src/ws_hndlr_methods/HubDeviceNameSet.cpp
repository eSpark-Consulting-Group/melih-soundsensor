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

WebSocketHandler::ResponceType WebSocketHandler::onHubDeviceNameSet( const gason::JsonValue& json,
                                                                     common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );
    const char* name = params( tokens::Name ).toString();
    const char* deviceId = params( tokens::SubId ).toString();
    auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !name || !deviceId || !devices ) {
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    std::vector<char> buff;
    if( !( devices && devices->getBlob( deviceId, buff ) ) ) {
        FUNC_LOGE( "Failed to get a current value for the key: %s", deviceId );
        // TODO: use more understandable for example 'not found'
        return std::make_tuple( errors::InvalidParams, StringType() );
    }

    gateways::BaseDeviceInfo deviceInfo;
    gateways::ObjectSerializer serializer;
    if( !serializer.deserialize( deviceId, std::move( buff ), deviceInfo ) ) {
        FUNC_LOGE( "Failed to deserialize the key: %s", deviceId );
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
    deviceInfo.m_name = std::string( name );

    buff = serializer.serialize( deviceInfo );
    if( buff.size() == 0 ) {
        FUNC_LOGE( "Failed to serialize the key: %s", deviceId );
        return std::make_tuple( errors::ApplicationError, StringType() );
    }

    if( !( devices && devices->setBlob( deviceId, buff ) ) ) {
        FUNC_LOGE( "Failed to write the key: %s", deviceId );
        return std::make_tuple( errors::ApplicationError, StringType() );
    }

    auto value = std::make_unique<common::StringValue>( name, strlen( name ) );
    auto cmd = std::make_unique<lua::responce::UpdateDevice>();
    if( cmd ) {
        cmd->m_deviceId.assign( deviceId );
        cmd->m_key.assign( common::resources::tokens::Name );
        cmd->m_value = std::move( value );

        m_client.send( lua::responce::BUS_NAME, std::move( cmd ) );
    }

    m_client.send(::bus::lua_event::BUS_NAME,
                  ::bus::lua_event::deviceUpdated( deviceId, ::common::resources::tokens::Name, name ) );

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma
