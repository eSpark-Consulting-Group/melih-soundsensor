#pragma once

#include "converters/LuaValueConverter.hpp"

namespace lua::module {

/**
 * Provides interface to invoke a given callable object using the content of Lua stack (arguments, return value).
 */
class LuaBindingExecutor {
public:
    explicit LuaBindingExecutor(::lua_State* lua )
    : m_lua( lua ) {}

    bool isValidStackSize( const int expectedSize ) {
        return ::lua_gettop( m_lua ) == expectedSize;
    }

    int returnError( const char* errString ) {
        return ::luaL_error( m_lua, errString );
    }

    /**
     * Invokes a given callable object with no return value with the arguments popped
     * from Lua stack.
     * @tparam _ArgTypes Types of arguments of a given callable object.
     * @param func Callable object with no return value.
     * @return Number of return values pushed to Lua stack.
     */
    template <typename... _ArgTypes>
    int invoke( std::function<void( _ArgTypes... )> func ) {
        if(::lua_gettop( m_lua ) != sizeof...( _ArgTypes ) ) {
            return ::luaL_error(
                m_lua, "Expected %d arguments, but received %d", sizeof...( _ArgTypes ), ::lua_gettop( m_lua ) );
        }
        func( std::move( converters::loadValue<_ArgTypes>( m_lua ) )... );
        return 0;
    }

    /**
     * Invokes a given callable object with the arguments popped from Lua stack.
     * @tparam _Res The type of return value of a given callable object.
     * @tparam _ArgTypes Types of arguments of a given callable object.
     * @param func Callable object.
     * @return Number of return values pushed to Lua stack.
     */
    template <typename _Res, typename... _ArgTypes>
    int invoke( std::function<_Res( _ArgTypes... )> func ) {
        if(::lua_gettop( m_lua ) != sizeof...( _ArgTypes ) ) {
            return ::luaL_error(
                m_lua, "Expected %d arguments, but received %d", sizeof...( _ArgTypes ), ::lua_gettop( m_lua ) );
        }

        converters::pushValue( m_lua, func( std::move( converters::loadValue<_ArgTypes>( m_lua ) )... ) );
        return 1;
    }

private:
    ::lua_State* m_lua;
};

}  // namespace lua::module
