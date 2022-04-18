#include <stdio.h>
#include <stdlib.h>
#include "Lua.h"
#include "bindings/LuaBindingHelper.h"
#include <LuaModuleWaterValve.h>
#include <WaterValveGateway.h>

#include <values/IntValue.h>
#include <values/FloatValue.h>
#include <values/RgbValue.h>
#include <values/ScalebleValue.h>
#include <DebugUtils.h>

#include <BusCommandRequestAllValue.h>
#include <BusCommandSetHandler.h>
#include <BusCommandSwitchValueUpdated.hpp>
#include <BusCommandSwitchValueSet.hpp>
#include <BusCommandWaterLeakValueUpdate.hpp>

namespace lua {

namespace module {

bool luaWaterValve::initialize() {
    return true;
}

luaWaterValve::luaWaterValve( Executor& exec, const char* name )
: IModule( exec, name ) {}

int luaWaterValve::regModule( lua_State* lua ) {
    static const auto bindArray =
        lua::createBindingsMap( luaL_Reg{ "join", luaWaterValve::luaJoin },
                                luaL_Reg{ "subscribe", luaWaterValve::luaSubscribe },
                                luaL_Reg{ "unsubscribe", luaWaterValve::luaUnsubscribe },
                                luaL_Reg{ "request_all_values", luaWaterValve::luaRequestAllValues },
                                luaL_Reg{ "set_water_switch_value", luaWaterValve::luaSetSwitchValue },
                                luaL_Reg{ "should_invert_control", luaWaterValve::luaShould_invert_control },
                                luaL_Reg{ "get_water_leak_sensors_count", luaWaterValve::luaGetWaterLeakSensorCount } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

int luaWaterValve::luaJoin( lua_State* lua ) {
    luaWaterValve& mod = *( (luaWaterValve*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );

    // const char* event = luaL_checkstring( lua, 1 );   // TODO how to use first param?
    const char* filename = luaL_checkstring( lua, 2 );

    int r = mod.m_exec.busJoin( mod, "extensions.plughub", filename );

    lua_pushboolean( lua, r );
    return 1;
}

int luaWaterValve::luaSubscribe( lua_State* lua ) {
    luaWaterValve& mod = *( (luaWaterValve*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );
    luaL_checktype( lua, 1, LUA_TSTRING );
    const char* handlerName = luaL_checkstring( lua, 1 );

    if( handlerName ) {
        mod.m_exec.send<bus::command::SetHandler>( WATER_VALVE_NAME, bus::BusAddrInvalid, handlerName );

        bool result{ true };
        lua_pushboolean( lua, result );
    }
    else {
        lua_pushnil( lua );
    }

    return 1;
}

int luaWaterValve::luaUnsubscribe( lua_State* lua ) {
    luaWaterValve& mod = *( (luaWaterValve*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 2 );
    luaL_checktype( lua, 1, LUA_TSTRING );
    const char* handlerName = luaL_checkstring( lua, 1 );

    if( handlerName ) {
        mod.m_exec.send<bus::command::SetHandler>( WATER_VALVE_NAME, bus::BusAddrInvalid, handlerName );

        bool result{ true };
        lua_pushboolean( lua, result );
    }
    else {
        lua_pushnil( lua );
    }

    return 1;
}
int luaWaterValve::luaRequestAllValues( lua_State* lua ) {
    luaWaterValve& mod = *( (luaWaterValve*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    auto bus_command = std::make_unique<bus::command::RequestAllValue>();
    if( bus_command ) {
        mod.m_exec.send( WATER_VALVE_NAME, std::move( bus_command ) );

        bool result{ true };
        lua_pushboolean( lua, result );
        return 1;
    }

    lua_pushnil( lua );
    return 1;
}

int luaWaterValve::luaGetWaterLeakSensorCount( lua_State* lua ) {
    luaWaterValve& mod = *( (luaWaterValve*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 1 );

    lua_pushinteger( lua, gateways::water_valve::Gateway::get().getWaterLeakSensorCount() );
    return 1;
}

int luaWaterValve::luaSetSwitchValue( lua_State* lua ) {
    luaWaterValve& mod = *( (luaWaterValve*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 1 );

    mod.m_exec.send(
        WATER_VALVE_NAME,
        std::make_unique<bus::command::SwitchValueSet>(::lua_toboolean( lua, 1 ), std::string{}, std::string{}, 0 ) );
    bool result{ true };
    lua_pushboolean( lua, result );
    return 1;
}

void luaWaterValve::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueUpdated& command ) {
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

void luaWaterValve::visit( const bus::BusAddr from,
                           const bus::BusAddr to,
                           bus::command::WaterLeakValueUpdate& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }

    if( !command.event().empty() ) {
        lua_createtable( lua, 0, 5 );

        lua_pushstring( lua, command.event().c_str() );
        lua_setfield( lua, -2, "event" );

        lua_pushboolean( lua, command.value() );
        lua_setfield( lua, -2, "value" );

        lua_pushinteger( lua, command.sensorIndex() );
        lua_setfield( lua, -2, "sensor_index" );

        m_exec.luaRet( 1 );
    }
    else {
        FUNC_LOGE( "WaterLeakValueUpdate. An event's name is empty" );
    }
}

int luaWaterValve::luaShould_invert_control( lua_State* lua ) {
    luaWaterValve& mod = *( (luaWaterValve*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );

    lua_settop( lua, 1 );

#if CONFIG_INVERT_WATER_VALVE_CONTROL
    bool result{ true };
#else
    bool result{ false };
#endif

    lua_pushboolean( lua, result );
    return 1;
}

}  // namespace module

}  // namespace lua
