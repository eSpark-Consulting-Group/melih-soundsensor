#include "bindings/LuaBindingHelper.h"

#include <algorithm>
#include <cstring>

#include "DebugUtils.h"

namespace lua {

namespace {

constexpr char ModulesMetaTable[] = "_MODULE_METATABLE";

int findBindingFunction( lua_State* lua, const char* methodName, int objIdx ) {
    lua_rawgeti( lua, objIdx, 1 );
    const auto bindings = lua_touserdata( lua, -1 );
    lua_pop( lua, 1 );

    if( !bindings ) {
        FUNC_LOGE( "Bindings is null. Method %s", methodName );
        return 0;
    }

    const auto& namesTofunctions = *reinterpret_cast<const BindingsMap*>( bindings );

    const auto begin = namesTofunctions.m_namesToFuncs;
    const auto end = begin + namesTofunctions.m_count;
    auto found = std::lower_bound( begin, end, methodName, []( const luaL_Reg& nameTofunc, const char* name ) {
        return strcmp( nameTofunc.name, name ) < 0;
    } );

    if( found != end && strcmp( found->name, methodName ) == 0 ) {
        lua_rawgeti( lua, objIdx, 2 );

        lua_pushcclosure( lua, found->func, 1 );

        // #ifdef CONFIG_SPIRAM
        // This code caches found method in module. TODO: Investigate, is it needed for rev1?
        lua_setfield( lua, objIdx, methodName );
        lua_getfield( lua, objIdx, methodName );
        // #endif

        return 1;
    }

    FUNC_LOGE( "Can't find function %s", methodName );

    return 0;
}

void createModuleBindingMetaTable( lua_State* lua ) {
    lua_newtable( lua );
    lua_pushcfunction( lua, []( lua_State* lua ) {
        const char* methodName = luaL_checkstring( lua, 2 );

        if( methodName == nullptr ) {
            FUNC_LOGE( "No method name in object" );
            return 0;
        }

        return findBindingFunction( lua, methodName, 1 );
    } );
    lua_setfield( lua, -2, "__index" );
    lua_setglobal( lua, ModulesMetaTable );
    lua_getglobal( lua, ModulesMetaTable );
}

}  // namespace

void BindingsMap::assign( size_t count, luaL_Reg* namesToFuncs ) {
    m_count = count;
    m_namesToFuncs = namesToFuncs;
    std::sort( namesToFuncs, namesToFuncs + count, []( const luaL_Reg& a, const luaL_Reg& b ) {
        return strcmp( a.name, b.name ) < 0;
    } );
}

int createModuleByBindingsMap( lua_State* lua, const BindingsMap& bindingsMap, void* data ) {
    lua_createtable( lua, 2, 0 );

    lua_pushlightuserdata( lua, const_cast<BindingsMap*>( &bindingsMap ) );
    lua_rawseti( lua, -2, 1 );

    lua_pushlightuserdata( lua, data );
    lua_rawseti( lua, -2, 2 );

    if( lua_getglobal( lua, ModulesMetaTable ) != LUA_TTABLE ) {
        lua_pop( lua, 1 );

        createModuleBindingMetaTable( lua );
    }

    lua_setmetatable( lua, -2 );

    return 1;
}

}  // namespace lua
