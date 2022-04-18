#pragma once

#include <lua.hpp>

namespace lua {

struct BindingsMap {
    size_t m_count = 0;
    luaL_Reg* m_namesToFuncs = nullptr;

    void assign( size_t count, luaL_Reg* namesToFuncs );
};

template <typename... Args>
struct BindingsMapImpl : BindingsMap {
    BindingsMapImpl( Args... args )
    : m_namesToFuncsStorage{ args... } {
        assign( sizeof...( Args ), m_namesToFuncsStorage );
    }

    luaL_Reg m_namesToFuncsStorage[sizeof...( Args )];
};

template <typename... Args>
const BindingsMapImpl<Args...> createBindingsMap( Args... namesToFuncs ) {
    return { namesToFuncs... };
}

int createModuleByBindingsMap( lua_State* lua, const BindingsMap& bindingsMap, void* data );

}  // namespace lua
