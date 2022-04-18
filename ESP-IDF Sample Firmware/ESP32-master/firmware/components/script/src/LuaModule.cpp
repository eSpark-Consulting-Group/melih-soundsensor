#include <LuaModule.h>
#include <Lua.h>
#include <stdlib.h>

#include <values/IntValue.h>
#include <values/Int64Value.hpp>
#include <values/BoolValue.h>
#include <values/FloatValue.h>
#include <values/StringValue.h>
#include <values/RgbValue.h>
#include <values/ScalebleValue.h>
#include <values/UserCodeValue.h>
#include <values/ArrayValue.h>
#include <IValueCast.h>

#include <CommonResources.h>

#include <DebugUtils.h>

//#define LUA_MODULE_DEBUG
#ifndef LUA_MODULE_DEBUG

#ifdef REMOTE_LOGW
#undef REMOTE_LOGW
#endif

#define REMOTE_LOGW( ... ) \
    do {                   \
    } while( false )

#endif

namespace lua {

const char luaDataKey[] = "__lua_binding_interface_data";
const char luaRunningPluginIdKey[] = "running_plugin_id";

IModule::IModule( Executor& exec, const char* name )
: m_exec( exec )
, m_name( name ) {
    m_exec.reg( *this, m_name );
}

void IModule::setPluginId( lua_State* luaState, const char* pluginId ) {
    lua_getglobal( luaState, luaDataKey );

    lua_pushstring( luaState, luaRunningPluginIdKey );
    lua_pushstring( luaState, pluginId );
    lua_rawset( luaState, -3 );
    lua_pop( luaState, 1 );
}

bool IModule::isLuaArray( lua_State* lua, int idx ) {
    if( !lua_istable( lua, idx ) ) {
        return false;
    }
    int arraySize = 0;
    ::lua_pushvalue( lua, idx );
    ::lua_pushnil( lua );
    while(::lua_next( lua, -2 ) ) {
        if( !::lua_isinteger( lua, -2 ) || lua_tointeger( lua, -2 ) != ++arraySize ) {
            lua_pop( lua, 3 );
            return false;
        }
        lua_pop( lua, 1 );
    }
    lua_pop( lua, 1 );
    return arraySize > 0;
}

std::string IModule::getPluginId( lua_State* luaState ) {
    lua_getglobal( luaState, luaDataKey );
    lua_pushstring( luaState, luaRunningPluginIdKey );

    lua_rawget( luaState, -2 );
    std::string scriptName;
    size_t size = 0;

    if( lua_isstring( luaState, -1 ) ) {
        auto value = lua_tolstring( luaState, -1, &size );
        if( value ) {
            scriptName = std::string{ value, size };
        }
    }

    lua_pop( luaState, 2 );
    return scriptName;
}

int IModule::getFieldAsInteger( lua_State* lua, const char* key ) {
    int value = 0;
    const char* str = luaL_checkstring( lua, -1 );
    if( str ) {
        value = std::strtol( str, NULL, 10 );
    }
    else {
        value = luaL_checkinteger( lua, -1 );
    }
    lua_pop( lua, 1 );

    return value;
}

std::string IModule::getStringField( lua_State* lua, const char* key ) {
    lua_pushstring( lua, key );
    lua_gettable( lua, -2 );  // get table[key]

    std::string result;
    if( lua_isstring( lua, -1 ) ) {
        result.assign( lua_tostring( lua, -1 ) );
    }
    else if( lua_isnil( lua, -1 ) ) {
        REMOTE_LOGW( __FUNCTION__, "The field: \"%s\" is nil", key );
    }
    else {
        REMOTE_LOGW( __FUNCTION__, "Type of value is not string. The field's name is \"%s\"", key );
    }
    lua_pop( lua, 1 );  // remove string from stack
    return result;
}

bool IModule::getBooleanField( lua_State* lua, const char* key, bool defaultValue ) {
    lua_pushstring( lua, key );
    lua_gettable( lua, -2 );  // get table[key]

    if( lua_isboolean( lua, -1 ) ) {
        bool value = lua_toboolean( lua, -1 );
        lua_pop( lua, 1 );  // remove from stack
        return value;
    }
    lua_pop( lua, 1 );  // remove from stack
    REMOTE_LOGW( __FUNCTION__, "Type of value is not boolean. The field's name is \"%s\"", key );
    return defaultValue;
}

int IModule::getIntegerField( lua_State* lua, const char* key ) {
    lua_pushstring( lua, key );
    lua_gettable( lua, -2 );  // get table[key]

    if( lua_isinteger( lua, -1 ) ) {
        int value = lua_tointeger( lua, -1 );
        lua_pop( lua, 1 );  // remove from stack
        return value;
    }
    lua_pop( lua, 1 );  // remove from stack
    REMOTE_LOGW( __FUNCTION__, "Type of value is not integer. The field's name is \"%s\"", key );
    return 0;
}

float IModule::getNumberField( lua_State* lua, const char* key ) {
    lua_pushstring( lua, key );
    lua_gettable( lua, -2 );  // get table[key]

    if( lua_isnumber( lua, -1 ) ) {
        float value = lua_tonumber( lua, -1 );
        lua_pop( lua, 1 );  // remove from stack
        return value;
    }
    lua_pop( lua, 1 );  // remove from stack
    REMOTE_LOGW( __FUNCTION__, "Type of value %s is not float", key );
    return 0;
}

std::vector<std::string> IModule::getArrayOfStringField( lua_State* lua, const char* key ) {
    std::vector<std::string> strArray;

    lua_pushstring( lua, key );
    lua_gettable( lua, -2 );
    if( lua_istable( lua, -1 ) ) {
        lua_pushvalue( lua, -1 );
        size_t len = lua_rawlen( lua, -1 );
        gateways::ItemInfo::EnumsArrayType enumValues;
        enumValues.reserve( len );
        for( int i = 1; len >= i; ++i ) {
            lua_pushnumber( lua, i );
            lua_gettable( lua, -2 );

            const char* result = lua_tostring( lua, -1 );
            if( result ) {
                strArray.emplace_back( result );
            }
            lua_pop( lua, 1 );  // stack: vector
        }
        lua_pop( lua, 1 );
    }
    lua_pop( lua, 1 );  // remove string from stack

    return strArray;
}

int IModule::popInteger( lua_State* lua ) {
    int value = 0;
    if( lua_isinteger( lua, 1 ) ) {
        value = lua_tointeger( lua, 1 );
        lua_remove( lua, 1 );  // remove from stack
        return value;
    }
    else if( lua_isnumber( lua, 1 ) ) {
        value = lua_tonumber( lua, 1 );
        lua_remove( lua, 1 );  // remove from stack
        return value;
    }
    else if( lua_isstring( lua, 1 ) ) {
        value = std::strtol( luaL_checkstring( lua, 1 ), NULL, 10 );
        lua_remove( lua, 1 );  // remove from stack
        return value;
    }
    REMOTE_LOGW( __FUNCTION__, "Type of value is not integer" );
    lua_remove( lua, 1 );  // remove from stack
    return 0;
}

float IModule::popNumber( lua_State* lua ) {
    float value = 0;
    if( lua_isinteger( lua, 1 ) ) {
        value = lua_tointeger( lua, 1 );
        lua_remove( lua, 1 );  // remove from stack
        return value;
    }
    else if( lua_isnumber( lua, 1 ) ) {
        value = lua_tonumber( lua, 1 );
        lua_remove( lua, 1 );  // remove from stack
        return value;
    }
    else if( lua_isstring( lua, 1 ) ) {
        value = std::strtol( luaL_checkstring( lua, 1 ), NULL, 10 );
        lua_remove( lua, 1 );  // remove from stack
        return value;
    }
    REMOTE_LOGW( __FUNCTION__, "Type of value is not number" );
    lua_remove( lua, 1 );  // remove from stack
    return 0;
}

bool IModule::jSonToLuaStack( lua_State* lua, gason::JsonValue& value ) {
    switch( value.getTag() ) {
    case gason::JSON_NUMBER:
        lua_pushnumber( lua, value.toNumber() );
        return true;

    case gason::JSON_STRING:
        if( !value.toString() )
            return false;
        lua_pushstring( lua, value.toString() );
        return true;

    case gason::JSON_ARRAY: {
        lua_newtable( lua );
        int arr_index = 0;
        for( auto i : value ) {
            lua_pushinteger( lua, arr_index );
            jSonToLuaStack( lua, i.value );
            lua_settable( lua, -3 );
            arr_index += 1;
        }
        return true;
    }

    case gason::JSON_OBJECT:
        lua_newtable( lua );
        for( auto i : value ) {
            if( i.value ) {
                if( jSonToLuaStack( lua, i.value ) ) {
                    lua_setfield( lua, -2, i.key );
                }
            }
            else {
                REMOTE_LOGW( __FUNCTION__, "value is EMPTY " );
            }
        }
        return true;

    case gason::JSON_TRUE:
    case gason::JSON_FALSE:
        lua_pushboolean( lua, value.toBool() );
        return true;

    case gason::JSON_NULL:
        // lua_pushnil( lua );
        // fall throught
    default:
        return false;
    }
}

common::IValue::UPtr IModule::createValueFromStack( lua_State* lua, int idx ) {
    switch(::lua_type( lua, idx ) ) {
    case LUA_TBOOLEAN:
        return std::make_unique<common::BoolValue>( lua_toboolean( lua, idx ) );

    case LUA_TNUMBER:
        if( lua_isinteger( lua, idx ) ) {
            return std::make_unique<common::IntValue>( lua_tointeger( lua, idx ) );
        }
        else {
            return std::make_unique<common::FloatValue>( lua_tonumber( lua, idx ) );
        }

    case LUA_TSTRING:
        return std::make_unique<common::StringValue>( lua_tostring( lua, idx ), lua_rawlen( lua, idx ) );

    case LUA_TTABLE: {
        if( isLuaArray( lua, idx ) ) {
            auto array = std::make_unique<::common::ArrayValue>();
            ::lua_pushvalue( lua, idx );
            ::lua_pushnil( lua );
            while(::lua_next( lua, -2 ) ) {
                ::lua_pushvalue( lua, -2 );
                if( auto itemValue = createValueFromStack( lua, -2 ) ) {
                    array->push_back( std::move( itemValue ) );
                }
                lua_pop( lua, 2 );
            }
            lua_pop( lua, 1 );
            return array;
        }
        else {
            auto tableValue = std::make_unique<common::TableValue>();
            ::lua_pushvalue( lua, idx );
            ::lua_pushnil( lua );
            while(::lua_next( lua, -2 ) ) {
                ::lua_pushvalue( lua, -2 );
                std::string str = lua_tostring( lua, -1 );
                lua_pop( lua, 1 );
                if( str.empty() ) {
                    continue;
                }
                if( auto itemValue = createValueFromStack( lua, -1 ) ) {
                    tableValue->insert( str, std::move( itemValue ) );
                }
                lua_pop( lua, 1 );
            }
            lua_pop( lua, 1 );
            return tableValue;
        }
    }

    default:
        REMOTE_LOGW( __FUNCTION__, "%sWrong type", ident.c_str() );
        return {};
    }
}

common::IValue::UPtr IModule::createValueFromStack( lua_State* lua ) {
    if( lua_gettop( lua ) < 1 ) {
        REMOTE_LOGI( __FUNCTION__, "Tries to operate an empty lua's stack" );
        return {};
    }
    return createValueFromStack( lua, 1 );
}

common::IValue::UPtr IModule::createValueFromField( lua_State* lua, const char* key ) {
    lua_pushstring( lua, key );
    lua_gettable( lua, -2 );  // get table[key]

    common::IValue::UPtr value = createValueFromStack( lua, -1 );
    lua_pop( lua, 1 );  // remove value from stack
    return value;
}

void PrintTable( lua_State* L ) {
    lua_pushnil( L );
    printf( "xt  \n" );
    while( lua_next( L, -2 ) != 0 ) {
        if( lua_isstring( L, -1 ) ) {
            if( lua_isstring( L, -2 ) ) {
                printf( "str field %s = %s\n", lua_tostring( L, -2 ), lua_tostring( L, -1 ) );
            }
            else {
                printf( "v = %s\n", lua_tostring( L, -1 ) );
            }
        }
        else if( lua_isnumber( L, -1 ) ) {
            if( lua_isstring( L, -2 ) ) {
                printf( "float field %s = %f\n", lua_tostring( L, -2 ), lua_tonumber( L, -1 ) );
            }
            else {
                printf( "v = %f\n", lua_tonumber( L, -1 ) );
            }
        }
        else if( lua_isinteger( L, -1 ) ) {
            if( lua_isstring( L, -2 ) ) {
                printf( "int field %s = " LUA_INTEGER_FMT "\n", lua_tostring( L, -2 ), lua_tointeger( L, -1 ) );
            }
            else {
                printf( "v = " LUA_INTEGER_FMT "\n", lua_tointeger( L, -1 ) );
            }
        }
        else if( lua_isboolean( L, -1 ) ) {
            if( lua_isstring( L, -2 ) ) {
                printf( "bool field %s = %d\n", lua_tostring( L, -2 ), (int)lua_isboolean( L, -1 ) );
            }
            else {
                printf( "value = %d\n", (int)lua_isboolean( L, -1 ) );
            }
        }
        else if( lua_istable( L, -1 ) ) {
            PrintTable( L );
        }
        else {
            printf( "other\n" );
        }
        lua_pop( L, 1 );
    }
}

void IModule::stackDump( lua_State* L ) {
    int i;
    int top = lua_gettop( L );
    for( i = 1; i <= top; i++ ) { /* repeat for each level */
        int t = lua_type( L, i );
        switch( t ) {
        case LUA_TSTRING: /* strings */
            printf( "xs [%d] = `%s'\n", i, lua_tostring( L, i ) );
            break;

        case LUA_TBOOLEAN: /* booleans */
            printf( "xb [%d] = ", i );
            printf( lua_toboolean( L, i ) ? "true" : "false" );
            break;

        case LUA_TNUMBER: /* numbers */
            printf( "xn [%d] = %g\n", i, lua_tonumber( L, i ) );
            break;

        case LUA_TTABLE:
            PrintTable( L );
            break;

        default: /* other values */
            printf( "xo [%d] = %s\n", i, lua_typename( L, t ) );
            break;
        }
        printf( "  \n" ); /* put a separator */
    }
    printf( "\n" ); /* end the listing */
}

void IModule::printLuaErrorDump( lua_State* L ) {
    // stack: err
    const char* err = lua_tostring( L, -1 );

    if( err ) {
        REMOTE_LOGE( __FUNCTION__, "Lua exucuting has been failed. Error:%s", err );
    }
    else {
        REMOTE_LOGE( __FUNCTION__, "Lua exucuting has been failed. Error UNDEFINED" );
    }
    lua_getglobal( L, "debug" );         // stack: err debug
    lua_getfield( L, -1, "traceback" );  // stack: err debug debug.traceback

    if( lua_pcall( L, 0, 1, 0 ) ) {
        err = lua_tostring( L, -1 );

        if( err ) {
            REMOTE_LOGE( __FUNCTION__, "Error in debug.traceback() call: %s", err );
        }
        else {
            REMOTE_LOGE( __FUNCTION__, "Error in debug.traceback() Error UNDEFINED" );
        }
    }
    else {
        const char* stackTrace = lua_tostring( L, -1 );

        if( stackTrace ) {
            REMOTE_LOGE( __FUNCTION__, "C++ stack traceback: %s", stackTrace );
        }
        else {
            REMOTE_LOGE( __FUNCTION__, "C++ stack traceback UNDEFINED" );
        }
    }
}

}  // namespace lua
