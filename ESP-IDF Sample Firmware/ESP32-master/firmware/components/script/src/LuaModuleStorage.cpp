#include <LuaModuleStorage.h>

#include <DebugUtils.h>
#include "Lua.h"
#include "bindings/LuaBindingHelper.h"

#include "LuaTableSerializer.h"
#include <BinaryFileStorageFactory.h>
#include <cstring>

//#define SIMPLE_MODTMR_DEBUG
#ifdef SIMPLE_MODSTRG_DEBUG

#include <esp_log.h>
#include "EspLogTags.h"

#define MODSTRG_LOGI( ... ) ESP_LOGI(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )
#define MODSTRG_LOGE( ... ) ESP_LOGE(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )
#define MODSTRG_LOGW( ... ) ESP_LOGW(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )

#else

#define MODSTRG_LOGI( ... ) \
    do {                    \
    } while( false )
#define MODSTRG_LOGE( ... ) \
    do {                    \
    } while( false )
#define MODSTRG_LOGW( ... ) \
    do {                    \
    } while( false )

#endif

namespace lua {

namespace module {

Storage::Storage( Executor& exec, const char* name, storage::BinaryFileStorageFactory& storageFactory )
: IModule( exec, name )
, m_storageFactory( storageFactory ) {}

bool Storage::initialize() {
    m_storages.clear();
    return true;
}

int Storage::regModule( lua_State* lua ) {
    m_storages.clear();

    static const auto bindArray = lua::createBindingsMap( luaL_Reg{ "get_bool", &Storage::lua_get_bool },
                                                          luaL_Reg{ "get_number", &Storage::lua_get_number },
                                                          luaL_Reg{ "get_string", &Storage::lua_get_string },
                                                          luaL_Reg{ "get_table", &Storage::lua_get_table },
                                                          luaL_Reg{ "set_bool", &Storage::lua_set_bool },
                                                          luaL_Reg{ "set_number", &Storage::lua_set_number },
                                                          luaL_Reg{ "set_string", &Storage::lua_set_string },
                                                          luaL_Reg{ "set_table", &Storage::lua_set_table },
                                                          luaL_Reg{ "exists", &Storage::lua_exists },
                                                          luaL_Reg{ "delete", &Storage::lua_delete },
                                                          luaL_Reg{ "delete_all", &Storage::lua_delete_all } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

void Storage::releaseStorage() {
    m_storages.clear();
}

int Storage::lua_get_bool( lua_State* lua ) {
    if( !lua_isstring( lua, 1 ) ) {
        MODSTRG_LOGE( "Wrong argument" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    std::int8_t value;
    if( self.getCurrentStorage().getInt8( key, value ) ) {
        lua_pushboolean( lua, value );
    }
    else {
        MODSTRG_LOGE( "Failed to load data with key: %s", key );
        lua_pushnil( lua );
    }
    return 1;
}

int Storage::lua_get_number( lua_State* lua ) {
    if( !lua_isstring( lua, 1 ) ) {
        MODSTRG_LOGE( "Wrong argument" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    lua_Number value;
    if( loadNumber( self, key, value ) ) {
        lua_pushnumber( lua, value );
    }
    else {
        float valueF;  // Fix for migration from 32bit lua to 64bit
        if( loadNumber( self, key, valueF ) ) {
            lua_pushnumber( lua, (lua_Number)valueF );
        }
        else {
            MODSTRG_LOGE( "Failed to load data with key: %s", key );
            lua_pushnil( lua );
        }
    }
    return 1;
}

int Storage::lua_get_string( lua_State* lua ) {
    if( !lua_isstring( lua, 1 ) ) {
        MODSTRG_LOGE( "Wrong argument" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    if( strlen( key ) == 0 ) {
        lua_pushnil( lua );
        return 1;
    }

    std::vector<char> value;
    if( self.getCurrentStorage().getBlob( key, value ) ) {
        lua_pushlstring( lua, value.data(), value.size() );
    }
    else {
        MODSTRG_LOGE( "Failed to load data with key: %s", key );
        lua_pushnil( lua );
    }
    return 1;
}

int Storage::lua_get_table( lua_State* lua ) {
    if( !lua_isstring( lua, 1 ) ) {
        MODSTRG_LOGE( "Wrong argument" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    if( strlen( key ) == 0 ) {
        lua_pushnil( lua );
        return 1;
    }

    std::vector<char> buff;
    TableSerializer serializer;
    if( !( self.getCurrentStorage().getBlob( key, buff ) && serializer.deserialize( lua, std::move( buff ) ) ) ) {
        lua_pushnil( lua );
        MODSTRG_LOGW( "Failed to load table with key: %s", key );
    }
    return 1;
}

int Storage::lua_set_bool( lua_State* lua ) {
    if( !( lua_isstring( lua, 1 ) && lua_isboolean( lua, 2 ) ) ) {
        MODSTRG_LOGE( "Wrong arguments" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    int value = lua_toboolean( lua, 2 );
    if( !self.getCurrentStorage().setInt8( key, value ) ) {
        MODSTRG_LOGE( "Failed to store data with key: %s", key );
    }
    lua_pushnil( lua );
    return 1;
}

int Storage::lua_set_number( lua_State* lua ) {
    if( !( lua_isstring( lua, 1 ) && lua_isnumber( lua, 2 ) ) ) {
        MODSTRG_LOGE( "Wrong arguments" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    lua_Number value = lua_tonumber( lua, 2 );
    if( !storeNumber( self, key, value ) ) {
        MODSTRG_LOGE( "Failed to store data with key: %s", key );
    }
    lua_pushnil( lua );
    return 1;
}

int Storage::lua_set_string( lua_State* lua ) {
    if( !( lua_isstring( lua, 1 ) && lua_isstring( lua, 2 ) ) ) {
        MODSTRG_LOGE( "Wrong argument" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    size_t value_size{ 0u };
    const char* value = lua_tolstring( lua, 2, &value_size );
    std::vector<char> buffer;
    buffer.reserve( value_size );
    std::copy( value, value + value_size, std::back_inserter( buffer ) );
    if( !self.getCurrentStorage().setBlob( key, buffer ) ) {
        MODSTRG_LOGE( "Failed to store data with key: %s", key );
    }

    lua_pushnil( lua );
    return 1;
}

int Storage::lua_set_table( lua_State* lua ) {
    if( !( lua_isstring( lua, 1 ) && lua_istable( lua, 2 ) ) ) {
        MODSTRG_LOGE( "Wrong argument" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    TableSerializer serializer;
    std::vector<char> vec = serializer.serialize( lua );
    if( !( !vec.empty() && self.getCurrentStorage().setBlob( key, vec ) ) ) {
        MODSTRG_LOGE( "Failed to store data with key: %s", key );
    }
    lua_pushnil( lua );
    return 1;
}

int Storage::lua_exists( lua_State* lua ) {
    if( !lua_isstring( lua, 1 ) ) {
        MODSTRG_LOGE( "Wrong argument" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    lua_pushboolean( lua, self.getCurrentStorage().exists( key ) );
    return 1;
}

int Storage::lua_delete( lua_State* lua ) {
    if( !lua_isstring( lua, 1 ) ) {
        MODSTRG_LOGE( "Wrong argument" );
        lua_pushnil( lua );
        return 1;
    }

    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* key = lua_tolstring( lua, 1, nullptr );
    if( !self.getCurrentStorage().remove( key ) ) {
        MODSTRG_LOGE( "Failed to remove key: %s", key );
    }
    lua_pushnil( lua );
    return 1;
}

int Storage::lua_delete_all( lua_State* lua ) {
    Storage& self = *reinterpret_cast<Storage*>( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    if( !self.getCurrentStorage().removeAll() ) {
        MODSTRG_LOGE( "Failed to remove all keys" );
    }
    lua_pushnil( lua );
    return 1;
}

bool Storage::storeNumber( Storage& self, const char* key, float& value ) {
    std::int32_t buffer;
    std::memcpy( &buffer, &value, sizeof( buffer ) );
    return self.getCurrentStorage().setInt32( key, buffer );
}

bool Storage::storeNumber( Storage& self, const char* key, double& value ) {
    std::int64_t buffer;
    std::memcpy( &buffer, &value, sizeof( buffer ) );
    return self.getCurrentStorage().setInt64( key, buffer );
}

bool Storage::loadNumber( Storage& self, const char* key, float& value ) {
    std::int32_t buffer;
    bool result{ false };
    if( self.getCurrentStorage().getInt32( key, buffer ) ) {
        std::memcpy( &value, &buffer, sizeof( buffer ) );
        result = true;
    }
    return result;
}

bool Storage::loadNumber( Storage& self, const char* key, double& value ) {
    std::int64_t buffer;
    bool result{ false };
    if( self.getCurrentStorage().getInt64( key, buffer ) ) {
        std::memcpy( &value, &buffer, sizeof( buffer ) );
        result = true;
    }
    return result;
}

storage::KeyValueStorage& Storage::getCurrentStorage() {
    auto it = m_storages.find( m_exec.getCurrentPluginID() );
    if( it == m_storages.end() ) {
        it = m_storages
                 .emplace( m_exec.getCurrentPluginID(),
                           m_storageFactory.openNamespace( m_exec.getCurrentPluginID().c_str() ) )
                 .first;
    }

    return *it->second;
}

}  // namespace module

}  // namespace lua
