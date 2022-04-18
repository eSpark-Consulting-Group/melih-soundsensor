#pragma once

/**
 * This file contains implementation of loadValue/pushValue support functions
 * used by LuaBindingContext. These functions perform conversion of common base types
 * to/from Lua stack values. Note that these functions should only work with generic
 * types. Any other types, such as gateways::zwave::devices::Device, znet::ZClass, etc.
 * should first be converted to ::common::IValue.
 */

#include <cstdlib>
#include <optional>

#include <lua.hpp>
#include <lauxlib.h>

#include "LuaModule.h"
#include "ValueToLuaGenerator.h"
#include "DebugUtils.h"
#include "IValueCast.h"
#include "values/ArrayValue.h"

namespace lua {
namespace module {
namespace converters {

template <typename T>
T loadValue(::lua_State* ); /* undefined */

template <>
inline std::string loadValue<std::string>(::lua_State* lua ) {
    std::string value;
    if( auto valuePtr = lua_tostring( lua, 1 ) ) {
        value.assign( valuePtr, ::lua_rawlen( lua, 1 ) );
    }
    lua_remove( lua, 1 );
    return value;
}

template <>
inline int64_t loadValue<int64_t>(::lua_State* lua ) {
    int64_t value = 0;
    if(::lua_isinteger( lua, 1 ) ) {
        value = lua_tointeger( lua, 1 );
    }
    else if(::lua_isnumber( lua, 1 ) ) {
        value = lua_tonumber( lua, 1 );
    }
    else if(::lua_isstring( lua, 1 ) ) {
        value = std::strtol( luaL_checkstring( lua, 1 ), NULL, 10 );
    }
    else {
        FUNC_LOGE( "Lua stack value is not an integer." );
    }
    lua_remove( lua, 1 );
    return value;
}

template <>
inline int loadValue<int>(::lua_State* lua ) {
    return loadValue<int64_t>( lua );
}

template <>
inline uint8_t loadValue<uint8_t>(::lua_State* lua ) {
    return loadValue<int64_t>( lua );
}

template <>
inline bool loadValue<bool>(::lua_State* lua ) {
    bool value = false;
    if( lua_isboolean( lua, 1 ) ) {
        value = lua_toboolean( lua, 1 );
    }
    else {
        FUNC_LOGE( "Lua stack value is not a bool." );
    }
    lua_remove( lua, 1 );
    return value;
}

template <typename _ElemT, typename _IValueCastT>
inline std::vector<_ElemT> loadVector(::lua_State* lua ) {
    std::vector<_ElemT> valueList;
    if( lua_istable( lua, 1 ) ) {
        ::lua_pushvalue( lua, 1 );
        ::lua_pushnil( lua );
        while(::lua_next( lua, -2 ) ) {
            if( auto value = ::common::iValueCast<_IValueCastT>(::lua::IModule::createValueFromStack( lua, -1 ) ) ) {
                valueList.push_back( value->get() );
            }
            lua_pop( lua, 1 );
        }
        lua_pop( lua, 1 );
    }
    lua_remove( lua, 1 );
    return valueList;
}

template <>
inline std::vector<uint8_t> loadValue<std::vector<uint8_t>>(::lua_State* lua ) {
    return loadVector<uint8_t, ::common::IntValue>( lua );
}

template <>
inline std::vector<std::string> loadValue<std::vector<std::string>>(::lua_State* lua ) {
    return loadVector<std::string, ::common::StringValue>( lua );
}

template <>
inline std::optional<std::string> loadValue<std::optional<std::string>>(::lua_State* lua ) {
    if( lua_isnil( lua, 1 ) ) {
        lua_remove( lua, 1 );
        return std::nullopt;
    }
    else {
        return loadValue<std::string>( lua );
    }
}

template <typename T>
std::unique_ptr<T> loadValueAndCast(::lua_State* lua ) {
    auto value = ::common::iValueCast<T>(::lua::IModule::createValueFromStack( lua ) );
    lua_remove( lua, 1 );
    return value;
}

template <>
inline std::unique_ptr<::common::ScalebleValue> loadValue<std::unique_ptr<::common::ScalebleValue>>(::lua_State* lua ) {
    return loadValueAndCast<::common::ScalebleValue>( lua );
}

template <>
inline std::unique_ptr<::common::FloatValue> loadValue<std::unique_ptr<::common::FloatValue>>(::lua_State* lua ) {
    return loadValueAndCast<::common::FloatValue>( lua );
}

template <>
inline std::unique_ptr<::common::RgbValue> loadValue<std::unique_ptr<::common::RgbValue>>(::lua_State* lua ) {
    return loadValueAndCast<::common::RgbValue>( lua );
}

template <>
inline std::unique_ptr<::common::TableValue> loadValue<std::unique_ptr<::common::TableValue>>(::lua_State* lua ) {
    return loadValueAndCast<::common::TableValue>( lua );
}

template <>
inline std::unique_ptr<::common::ArrayValue> loadValue<std::unique_ptr<::common::ArrayValue>>(::lua_State* lua ) {
    return loadValueAndCast<::common::ArrayValue>( lua );
}

template <>
inline std::unique_ptr<::common::IValue> loadValue<std::unique_ptr<::common::IValue>>(::lua_State* lua ) {
    auto value = ::lua::IModule::createValueFromStack( lua );
    lua_remove( lua, 1 );
    return value;
}

inline void pushValue(::lua_State* lua, bool value ) {
    FUNC_LOGD( "{%s}", value ? "true" : "false" );
    ::lua_pushboolean( lua, value );
}

inline void pushValue(::lua_State* lua, int value ) {
    FUNC_LOGD( "{%d}", value );
    ::lua_pushinteger( lua, value );
}

inline void pushValue(::lua_State* lua, const std::string& value ) {
    FUNC_LOGD( "{\"%s\"}", value.c_str() );
    ::lua_pushstring( lua, value.c_str() );
}

template <typename _ElemT>
inline void pushValue(::lua_State* lua, std::vector<_ElemT> valueList ) {
    FUNC_LOGD( "{[" );
    lua_newtable( lua );
    for( size_t i = 0; i < valueList.size(); ++i ) {
        ::lua_pushnumber( lua, i + 1 );
        pushValue( lua, valueList[i] );
        ::lua_settable( lua, -3 );
    }
    FUNC_LOGD( "]}" );
}

inline void pushValue(::lua_State* lua, std::shared_ptr<::common::IValue> value ) {
    if( value ) {
        auto valueJson = value->toJson( "value" );
        FUNC_LOGD( "%s", valueJson.c_str() );
        ::lua::ValueToLuaGenerator converter{ lua };
        value->accept( converter );
    }
    else {
        FUNC_LOGD( "{nil}" );
        ::lua_pushnil( lua );
    }
}

}  // namespace converters
}  // namespace module
}  // namespace lua
