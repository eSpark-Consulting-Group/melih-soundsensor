#ifdef CONFIG_CLOUD_DEVICES

#include "LuaModuleCloud.h"

#include <Lua.h>
#include "IValueCast.h"
#include <DebugUtils.h>

#include <BusCommandSetHandler.h>
#include "lua/responce/HubSendBroadcast.h"
#include <cloud/responses/BusCommandHandleEventCloudResponseCommand.h>
#include <cloud/responses/BusCommandHandleEventCloudDeviceAdded.h>
#include <cloud/responses/BusCommandHandleEventCloudDeviceRemoved.h>
#include <cloud/responses/BusCommandHandleEventCloudDeviceUpdated.h>

#include <GatewayUtils.h>
#include <IGateway.h>
#include "ColudDevicesManager.h"

namespace cloud {

bool LuaModule::initialize() {
    return true;
}

LuaModule::LuaModule( lua::Executor& exec, const char* name )
: ParentType( exec, name ) {}

int LuaModule::regModule( lua_State* lua ) {
    struct bind {
        const char* name;
        int ( *function )( lua_State* lua );
        void registerFunction( lua_State* lua, void* p ) const {
            lua_pushlightuserdata( lua, p );
            lua_pushcclosure( lua, function, 1 );
            lua_setfield( lua, -2, name );
        }
    };

    const bind bindArray[] = { { "join", LuaModule::luaJoin },
                               { "request_all_values", LuaModule::luaRequestAllValues },
                               { "get_all_devices_ids", LuaModule::getAllDevicesIds },
                               { "get_device", LuaModule::getDevice },
                               { "send_command", LuaModule::sendCommand },
                               { "subscribe", LuaModule::luaSubscribe },
                               { "unsubscribe", LuaModule::luaUnsubscribe } };

    const size_t bindArraySize = sizeof( bindArray ) / sizeof( bindArray[0] );

    lua_createtable( lua, 0, bindArraySize );
    for( size_t i = 0; i < bindArraySize; i++ ) {
        bindArray[i].registerFunction( lua, (void*)this );
    }

    return 1;
}

int LuaModule::luaJoin( lua_State* lua ) {
    LuaModule& mod = *( (LuaModule*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );
    const char* filename = luaL_checkstring( lua, 2 );
    int r = mod.m_exec.busJoin( mod, "extensions.cloud_devices", filename );
    lua_pushboolean( lua, r );
    return 1;
}

int LuaModule::getAllDevicesIds( lua_State* luaState ) {
    ::lua_createtable( luaState, 0, 0 );
    return 1;
}

int LuaModule::sendCommand( lua_State* luaState ) {
    return 1;
}

int LuaModule::getDevice( lua_State* luaState ) {
    if( lua_gettop( luaState ) != 1 ) {
        return luaL_error( luaState, "expecting exactly 1 arguments" );
    };

    const char* device_id = lua_tolstring( luaState, 1, nullptr );
    if( device_id == nullptr ) {
        return 0;
    }

    lua_createtable( luaState, 0, 1 );

    std::string id( device_id );

    gateways::IGateway::AddingDeviceStatus addingStatus;
    gateways::cloud::Device device = gateways::cloud::ColudDevicesManager::get().getDeviceInfo( id, addingStatus );

    if( addingStatus == gateways::IGateway::AddingDeviceStatus::OK ) {
        lua_pushstring( luaState, device.getDeviceId() );
        lua_setfield( luaState, -2, common::resources::lua_events::GatewayId );

        lua_pushstring( luaState, device.getName() );
        lua_setfield( luaState, -2, common::resources::tokens::Name );

        lua_pushstring( luaState, device.getCategory() );
        lua_setfield( luaState, -2, common::resources::tokens::Category );

        lua_pushstring( luaState, device.getSubCategory() );
        lua_setfield( luaState, -2, common::resources::tokens::SubCategory );

        lua_pushstring( luaState, device.getType() );
        lua_setfield( luaState, -2, common::resources::lua_events::ValueType );

        lua_pushstring( luaState, device.getTypeId() );
        lua_setfield( luaState, -2, common::resources::lua_events::DeviceTypeId );

        lua_createtable( luaState, 0, 1 );
        lua_pushstring( luaState, "info" );
        lua_createtable( luaState, 0, 2 );

        lua_pushstring( luaState, device.getInfo().account.c_str() );
        lua_setfield( luaState, -2, "account" );

        lua_pushstring( luaState, device.getInfo().vendor.c_str() );
        lua_setfield( luaState, -2, "vendor" );
        lua_settable( luaState, -3 );

        lua_pushstring( luaState, device.getSecurity() );
        lua_setfield( luaState, -2, common::resources::tokens::Security );

        lua_pushstring( luaState, id.c_str() );
        lua_setfield( luaState, -2, "id" );

        lua_pushstring( luaState, "capabilities" );

        const auto capabilities = device.getCapabilities();

        lua_createtable( luaState, capabilities.size(), 0 );

        int index = 0;
        for( auto capability : capabilities ) {
            index++;
            lua_pushstring( luaState, capability.c_str() );
            lua_seti( luaState, -2, index );
        }

        lua_settable( luaState, -3 );

        lua_pushboolean( luaState, device.batteryPowered() );
        lua_setfield( luaState, -2, common::resources::lua_events::BatteryPowered );

        lua_pushboolean( luaState, device.isReachable() );
        lua_setfield( luaState, -2, common::resources::tokens::Reachable );
    }

    return 1;
}

int LuaModule::luaSubscribe( lua_State* lua ) {
    LuaModule& mod = *( (LuaModule*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );
    luaL_checktype( lua, 1, LUA_TSTRING );
    const char* handlerName = luaL_checkstring( lua, 1 );

    if( handlerName ) {
        mod.m_exec.send<bus::command::SetHandler>( CLOUD_NAME, bus::BusAddrInvalid, handlerName );

        bool result{ true };
        lua_pushboolean( lua, result );
    }
    else {
        lua_pushnil( lua );
    }

    return 1;
}

int LuaModule::luaUnsubscribe( lua_State* lua ) {
    LuaModule& mod = *( (LuaModule*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );
    luaL_checktype( lua, 1, LUA_TSTRING );
    const char* handlerName = luaL_checkstring( lua, 1 );

    if( handlerName ) {
        mod.m_exec.send<bus::command::SetHandler>( CLOUD_NAME, bus::BusAddrInvalid, handlerName );

        bool result{ true };
        lua_pushboolean( lua, result );
    }
    else {
        lua_pushnil( lua );
    }

    return 1;
}

int LuaModule::luaRequestAllValues( lua_State* lua ) {
    LuaModule& mod = *( (LuaModule*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    const auto handlerName = std::string();
    mod.m_exec.send<bus::command::SetHandler>( CLOUD_NAME, bus::BusAddrInvalid, handlerName );

    bool result{ true };
    lua_pushboolean( lua, result );
    return 1;
}  // namespace module

void LuaModule::visit( const bus::BusAddr from,
                       const bus::BusAddr to,
                       responses::HandleEventCloudResponseCommand& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }

    if( command.event() == "command_response" ) {
        FUNC_LOGE( "command_response" );

        lua_createtable( lua, 0, 2 );

        lua_pushstring( lua, command.event().c_str() );
        lua_setfield( lua, -2, "event" );

        lua_pushstring( lua, command.commandId().c_str() );
        lua_setfield( lua, -2, common::resources::lua_events::CommandId );

        lua_createtable( lua, 0, 1 );
        lua_pushstring( lua, command.result().c_str() );
        lua_setfield( lua, -2, "result" );
        lua_settable( lua, -3 );

        lua_createtable( lua, 0, 1 );
        lua_pushstring( lua, command.error().c_str() );
        lua_setfield( lua, -2, "error" );
        lua_settable( lua, -3 );

        m_exec.luaRet( 1 );
    }
    else {
        FUNC_LOGE( "Error response command" );
    }
}

void LuaModule::visit( const bus::BusAddr from,
                       const bus::BusAddr to,
                       responses::HandleEventCloudDeviceAdded& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }

    if( command.event() == "device_added" ) {
        lua_createtable( lua, 0, 2 );

        lua_pushstring( lua, command.event().c_str() );
        lua_setfield( lua, -2, "event" );

        lua_pushstring( lua, command.id().c_str() );
        lua_setfield( lua, -2, "id" );

        m_exec.luaRet( 1 );
    }
    else {
        FUNC_LOGE( "Error added command" );
    }
}

void LuaModule::visit( const bus::BusAddr from,
                       const bus::BusAddr to,
                       responses::HandleEventCloudDeviceRemoved& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }

    if( command.event() == "device_removed" ) {
        lua_createtable( lua, 0, 2 );

        lua_pushstring( lua, command.event().c_str() );
        lua_setfield( lua, -2, "event" );

        lua_pushstring( lua, command.id().c_str() );
        lua_setfield( lua, -2, "id" );

        m_exec.luaRet( 1 );
    }
    else {
        FUNC_LOGE( "Error removed command" );
    }
}

void LuaModule::visit( const bus::BusAddr from,
                       const bus::BusAddr to,
                       responses::HandleEventCloudDeviceUpdated& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }

    if( command.event() == "device_updated" ) {
        lua_createtable( lua, 0, 1 );

        lua_pushstring( lua, command.event().c_str() );
        lua_setfield( lua, -2, "event" );

        lua_pushstring( lua, command.id().c_str() );
        lua_setfield( lua, -2, "id" );

        for( std::pair<std::string, std::string> field : command.fields() ) {
            lua_pushstring( lua, field.second.c_str() );
            lua_setfield( lua, -2, field.first.c_str() );
        }

        m_exec.luaRet( 1 );
    }
    else {
        FUNC_LOGE( "Error updated command" );
    }
}

}  // namespace cloud

#endif  // CONFIG_CLOUD_DEVICES
