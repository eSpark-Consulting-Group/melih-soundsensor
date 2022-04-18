#include "LuaModuleHouseModes.h"
#include "bindings/LuaHouseModesBindings.hpp"
#include "bindings/LuaBindingHelper.h"

#include <BusCommandModesSwitched.h>

namespace lua {

namespace module {

HouseModes::HouseModes( Executor& exec, const char* name )
: IModule( exec, name ) {}

int HouseModes::regModule( lua_State* lua ) {
    static const auto bindArray = lua::createBindingsMap(
        luaL_Reg{ "switch", bindings::luaBinding<bindings::house_modes::Switch> },
        luaL_Reg{ "cancel_switch", bindings::luaBinding<bindings::house_modes::CancelSwitch> },
        luaL_Reg{ "get_current", bindings::luaBinding<bindings::house_modes::GetCurrent> },
        luaL_Reg{ "get_modes", bindings::luaBinding<bindings::house_modes::GetModes> },
        luaL_Reg{ "get_mode", bindings::luaBinding<bindings::house_modes::GetMode> },
        luaL_Reg{ "find_mode_by_name", bindings::luaBinding<bindings::house_modes::FindModeByName> },
        luaL_Reg{ "subscribe", bindings::luaBinding<bindings::house_modes::Subscribe> } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

void HouseModes::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesSwitched& command ) {
    lua_State* lua = m_exec.lua();
    if( !lua ) {
        FUNC_LOGE( "The lua's state is empty" );
        return;
    }

    lua_createtable( lua, 0, 2 );

    lua_pushstring( lua, "house_modes_switched" );
    lua_setfield( lua, -2, "event" );

    lua_pushstring( lua, command.m_from.c_str() );
    lua_setfield( lua, -2, "from" );

    lua_pushstring( lua, command.m_to.c_str() );
    lua_setfield( lua, -2, "to" );

    lua_pushstring( lua, command.m_status.c_str() );
    lua_setfield( lua, -2, "status" );

    lua_pushboolean( lua, command.m_switchDelay );
    lua_setfield( lua, -2, "switch_delay" );

    m_exec.luaRet( 1 );
}

}  // namespace module

}  // namespace lua
