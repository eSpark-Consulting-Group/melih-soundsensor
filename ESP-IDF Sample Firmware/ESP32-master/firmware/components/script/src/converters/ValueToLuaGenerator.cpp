#include "converters/ValueToLuaGenerator.h"

#include "values/BoolValue.h"
#include "values/IntValue.h"
#include "values/Int64Value.hpp"
#include "values/FloatValue.h"
#include "values/RgbValue.h"
#include "values/ScalebleValue.h"
#include "values/StringValue.h"
#include "values/ArrayValue.h"
#include "values/TableValue.h"
#include "values/UserCodeValue.h"
#include "values/TokenValue.h"

#include <lua.hpp>

#include "CommonResources.h"
#include "DebugUtils.h"
#include <esp_log.h>
#include "EspLogTags.h"

//#define SIMPLE_VAL2LUA_DEBUG
#ifdef SIMPLE_VAL2LUA_DEBUG
#define VAL2LUA_LOGD( ... ) ESP_LOGD(::esp_log_tags::VALUE_TO_LUA, __VA_ARGS__ )
#else

#define VAL2LUA_LOGD( ... ) \
    do {                    \
    } while( false )
#endif

namespace lua {

ValueToLuaGenerator::ValueToLuaGenerator( lua_State* luaState )
: m_luaState( luaState ) {}

void ValueToLuaGenerator::visit( const ::common::FloatValue& value ) {
    VAL2LUA_LOGD( "FloatValue %f", value.get() );
    ::lua_pushnumber( m_luaState, value.get() );
}

void ValueToLuaGenerator::visit( const ::common::ScalebleValue& value ) {
    if( !value.getScale().empty() ) {
        VAL2LUA_LOGD( "ScalebleValue scale = %s", value.getScale().c_str() );
    }
    else {
        VAL2LUA_LOGD( "ScalebleValue scale EMPTY" );
    }
    VAL2LUA_LOGD( "ScalebleValue %f", value.get() );
    ::lua_createtable( m_luaState, 0, 2 );
    ::lua_pushstring( m_luaState, value.getScale().c_str() );
    ::lua_setfield( m_luaState, -2, ::common::resources::tokens::Scale );
    ::lua_pushnumber( m_luaState, value.get() );
    ::lua_setfield( m_luaState, -2, ::common::resources::tokens::Value );
}

void ValueToLuaGenerator::visit( const ::common::IntValue& value ) {
    VAL2LUA_LOGD( "IntValue %d", (int)value.get() );
    ::lua_pushinteger( m_luaState, (int)value.get() );
}

void ValueToLuaGenerator::visit( const ::common::Int64Value& value ) {
    VAL2LUA_LOGD( "Int64Value %" PRId64, value.get() );
    ::lua_pushinteger( m_luaState, value.get() );
}

void ValueToLuaGenerator::visit( const ::common::BoolValue& value ) {
    VAL2LUA_LOGD( "BoolValue %d", (int)value.get() );
    ::lua_pushboolean( m_luaState, (int)value.get() );
}

void ValueToLuaGenerator::visit( const ::common::StringValue& value ) {
    VAL2LUA_LOGD( "StringValue %s", value.get().c_str() );
    ::lua_pushstring( m_luaState, value.get().c_str() );
}

void ValueToLuaGenerator::visit( const ::common::TokenValue& value ) {
    VAL2LUA_LOGD( "TokenValue %s", value.get().c_str() );
    ::lua_pushstring( m_luaState, value.get().c_str() );
}

void ValueToLuaGenerator::visit( const ::common::RgbValue& value ) {
    auto it = value.get().begin();

    ::lua_createtable( m_luaState, 0, value.getComponentsCount() );
    for( ; it != value.get().end(); ++it ) {
        if(::common::RgbValue::componentIdToStr( ( *it ).color ) == nullptr ) {
            FUNC_LOGE( "Null component_id in RgbValue!" );
            continue;
        }
        ::lua_pushinteger( m_luaState, (int)( *it ).value );
        ::lua_setfield( m_luaState, -2, ::common::RgbValue::componentIdToStr( ( *it ).color ) );
    }
}

void ValueToLuaGenerator::visit( const ::common::NotificationValue& ) {
    // TODO
    FUNC_LOGW( ">>>> ValueToLuaGenerator for ::common::NotificationValue is not implemented <<<<" );
}

void ValueToLuaGenerator::visit( const ::common::ArrayValue& array ) {
    lua_newtable( m_luaState );
    size_t i = 1;
    for( const auto& item : array ) {
        VAL2LUA_LOGD( "ArrayValue row %d", i );
        ::lua_pushinteger( m_luaState, i++ );
        item->accept( *this );
        ::lua_settable( m_luaState, -3 );
    }
}

void ValueToLuaGenerator::visit( const ::common::TableValue& value ) {
    lua_newtable( m_luaState );
    for( const auto& val : value ) {
        if( val.second ) {
            val.second->accept( *this );
            ::lua_setfield( m_luaState, -2, val.first.c_str() );
        }
        else {
            VAL2LUA_LOGD( "Null" );
        }
        VAL2LUA_LOGD( "TableValue key %s", val.first.c_str() );
    }
}

void ValueToLuaGenerator::visit( const ::common::UserCodeValue& value ) {
    lua_newtable( m_luaState );

    ::lua_pushstring( m_luaState, value.getCode().c_str() );
    ::lua_setfield( m_luaState, -2, ::common::resources::tokens::Code );

    ::lua_pushstring( m_luaState, value.getName().c_str() );
    ::lua_setfield( m_luaState, -2, ::common::resources::tokens::Name );
}

}  // namespace lua
