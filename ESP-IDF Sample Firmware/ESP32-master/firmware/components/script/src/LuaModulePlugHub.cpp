#include <stdio.h>
#include <stdlib.h>
#include "Lua.h"
#include "bindings/LuaBindingHelper.h"
#include <LuaModulePlugHub.h>

#include <values/IntValue.h>
#include <values/FloatValue.h>
#include <values/RgbValue.h>
#include <values/ScalebleValue.h>
#include <DebugUtils.h>

#include <BusCommandRequestAllValue.h>
#include <BusCommandSetHandler.h>
#include <BusCommandSwitchValueUpdated.hpp>
#include <BusCommandSwitchValueSet.hpp>
#include <BusCommandSetConsumedEnergyMeter.hpp>
#include <BusCommandElectricMeterValuesUpdated.hpp>
#include <BusCommandResetEnergyMeter.hpp>

namespace lua {

namespace module {

bool luaPlugHub::initialize() {
    return true;
}

luaPlugHub::luaPlugHub( Executor& exec, const char* name )
: IModule( exec, name ) {}

int luaPlugHub::regModule( lua_State* lua ) {
    static const auto bindArray =
        lua::createBindingsMap( luaL_Reg{ "join", luaPlugHub::luaJoin },
                                luaL_Reg{ "subscribe", luaPlugHub::luaSubscribe },
                                luaL_Reg{ "unsubscribe", luaPlugHub::luaUnsubscribe },
                                luaL_Reg{ "request_all_values", luaPlugHub::luaRequestAllValues },
                                luaL_Reg{ "set_switch_value", luaPlugHub::luaSetSwitchValue },
                                luaL_Reg{ "reset_consumed_energy_meter", luaPlugHub::luaResetConsumedEnergyMeter },
                                luaL_Reg{ "set_consumed_energy_meter", luaPlugHub::luaSetConsumedEnergyMeter } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

int luaPlugHub::luaJoin( lua_State* lua ) {
    luaPlugHub& mod = *( (luaPlugHub*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );

    // const char* event = luaL_checkstring( lua, 1 );   // TODO how to use first param?
    const char* filename = luaL_checkstring( lua, 2 );

    int r = mod.m_exec.busJoin( mod, "extensions.plughub", filename );

    lua_pushboolean( lua, r );
    return 1;
}

int luaPlugHub::luaSubscribe( lua_State* lua ) {
    luaPlugHub& mod = *( (luaPlugHub*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );
    luaL_checktype( lua, 1, LUA_TSTRING );
    const char* handlerName = luaL_checkstring( lua, 1 );

    if( handlerName ) {
        mod.m_exec.send<bus::command::SetHandler>( PLUG_HUB_NAME, bus::BusAddrInvalid, handlerName );

        bool result{ true };
        lua_pushboolean( lua, result );
    }
    else {
        lua_pushnil( lua );
    }

    return 1;
}

int luaPlugHub::luaUnsubscribe( lua_State* lua ) {
    luaPlugHub& mod = *( (luaPlugHub*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );
    luaL_checktype( lua, 1, LUA_TSTRING );
    const char* handlerName = luaL_checkstring( lua, 1 );

    if( handlerName ) {
        mod.m_exec.send<bus::command::SetHandler>( PLUG_HUB_NAME, bus::BusAddrInvalid, handlerName );

        bool result{ true };
        lua_pushboolean( lua, result );
    }
    else {
        lua_pushnil( lua );
    }

    return 1;
}
int luaPlugHub::luaRequestAllValues( lua_State* lua ) {
    FUNC_LOGD( "luaPlugHub::luaRequestAllValues" );
    luaPlugHub& mod = *( (luaPlugHub*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    auto bus_command = std::make_unique<bus::command::RequestAllValue>();
    if( bus_command ) {
        mod.m_exec.send( PLUG_HUB_NAME, std::move( bus_command ) );

        bool result{ true };
        lua_pushboolean( lua, result );
        return 1;
    }

    lua_pushnil( lua );
    return 1;
}

int luaPlugHub::luaResetConsumedEnergyMeter( lua_State* lua ) {
    luaPlugHub& mod = *( (luaPlugHub*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 1 );

    mod.m_exec.send( PLUG_HUB_NAME,
                     std::make_unique<bus::command::ResetEnergyMeter>( std::string{}, std::string{}, 0 ) );
    bool result{ true };
    lua_pushboolean( lua, result );
    return 1;
}

int luaPlugHub::luaSetConsumedEnergyMeter( lua_State* lua ) {
    luaPlugHub& mod = *( (luaPlugHub*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 1 );

    mod.m_exec.send( PLUG_HUB_NAME,
                     std::make_unique<bus::command::SetConsumedEnergyMeter>(
                         ::lua_tonumber( lua, 1 ), std::string{}, std::string{}, 0 ) );
    bool result{ true };
    lua_pushboolean( lua, result );
    return 1;
}

int luaPlugHub::luaSetSwitchValue( lua_State* lua ) {
    luaPlugHub& mod = *( (luaPlugHub*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 1 );

    mod.m_exec.send(
        PLUG_HUB_NAME,
        std::make_unique<bus::command::SwitchValueSet>(::lua_toboolean( lua, 1 ), std::string{}, std::string{}, 0 ) );
    bool result{ true };
    lua_pushboolean( lua, result );
    return 1;
}

void luaPlugHub::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueUpdated& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }

    if( !command.event().empty() ) {
        lua_createtable( lua, 0, 2 );

        lua_pushstring( lua, command.event().c_str() );
        lua_setfield( lua, -2, "event" );

        lua_pushboolean( lua, command.value() );
        lua_setfield( lua, -2, "value" );

        m_exec.luaRet( 1 );
    }
    else {
        FUNC_LOGE( "SwitchValueUpdated. An event's name is empty" );
    }
}

void luaPlugHub::visit( const bus::BusAddr from,
                        const bus::BusAddr to,
                        bus::command::ElectricMeterValuesUpdated& command ) {
    FUNC_LOGD( "bus::command::ElectricMeterValuesUpdated" );
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }

    if( !command.event().empty() ) {
        lua_createtable( lua, 0, 5 );

        lua_pushstring( lua, command.event().c_str() );
        lua_setfield( lua, -2, "event" );

        lua_pushnumber( lua, command.voltage() );
        lua_setfield( lua, -2, "voltage" );

        lua_pushnumber( lua, command.current() );
        lua_setfield( lua, -2, "current" );

        lua_pushnumber( lua, command.power() );
        lua_setfield( lua, -2, "power" );

        lua_pushnumber( lua, command.consumedEnergy() );
        lua_setfield( lua, -2, "consumed_energy" );

        m_exec.luaRet( 1 );
    }
    else {
        FUNC_LOGE( "ElectricMeterValuesUpdated. An event's name is empty" );
    }
}

}  // namespace module

}  // namespace lua
