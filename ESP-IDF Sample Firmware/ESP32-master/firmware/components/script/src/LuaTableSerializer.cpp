#include "LuaTableSerializer.h"
#include <cstdint>
#include <vector>
#include <utility>
#include <algorithm>
#include "DebugUtils.h"
#include "LuaTableSerializer.h"

namespace lua {

namespace {

constexpr char SERIALIZER_MAGIC = 0xCC;
constexpr char SERIALIZER_TABLE_BEGIN = 0x55;
constexpr char SERIALIZER_TABLE_END = 0xAA;

using type_id_t = char;
using item_size_t = std::uint16_t;
using integer_t = std::int32_t;  // TODO: use LUA_INTEGER instead of std::int32_t after storage migration is done
using float_t = LUA_NUMBER;

enum class TYPE_ID : type_id_t { BOOL = 0u, INTEGER, FLOAT, STRING, TABLE, NIL };

template <typename T, typename std::enable_if<std::is_same<T, bool>::value, int>::type = 0>
constexpr TYPE_ID get_type_id( T ) {
    return TYPE_ID::BOOL;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, int>::type = 0>
constexpr TYPE_ID get_type_id( T ) {
    return TYPE_ID::INTEGER;
}

template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
constexpr TYPE_ID get_type_id( T ) {
    return TYPE_ID::FLOAT;
}

}  // namespace

TableSerializer::SerializedParams TableSerializer::serialize( lua_State* lua ) {
    if( !lua_istable( lua, -1 ) ) {
        FUNC_LOGE( "Wrong argument type" );
        return {};
    }

    m_buff = std::make_unique<common::SerializationBuffer>();
    m_buff->write( SERIALIZER_MAGIC );
    if( !serialize_table( lua ) ) {
        FUNC_LOGE( "Can't serialize table" );
        return {};
    }
    return m_buff->extract();
}

bool TableSerializer::deserialize( lua_State* lua, SerializedParams buff ) {
    m_buff = std::make_unique<common::SerializationBuffer>();
    m_buff->init( std::move( buff ) );
    if( !( m_buff->can_read( sizeof( char ) ) && m_buff->read<char>() == SERIALIZER_MAGIC ) ) {
        m_buff.reset();
        return false;
    }
    return deserialize_table( lua );
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type>
void TableSerializer::write_item( T item ) {
    m_buff->write( static_cast<type_id_t>( get_type_id( item ) ) );
    m_buff->write( item );
}

void TableSerializer::write_string( const void* str, std::size_t length ) {
    m_buff->write( static_cast<type_id_t>( TYPE_ID::STRING ) );
    m_buff->write<item_size_t>( length );
    m_buff->write( str, length );
}

bool TableSerializer::serialize_table( lua_State* lua ) {
    m_buff->write( SERIALIZER_TABLE_BEGIN );
    lua_pushnil( lua );
    bool result{ true };
    while( lua_next( lua, -2 ) != 0 ) {
        common::SerializationBuffer::position_t position = m_buff->getPosition();
        // Key cannot be NIL
        result = !lua_isnil( lua, -2 );
        // Try to serialize key/value pair. In case of error silently ignore it and restore position. This will skip
        // serialization of unsupported data
        if( !( result && serialize_item( lua, -2 ) && serialize_item( lua, -1 ) ) ) {
            m_buff->restorePosition( position );
        }
        // Pop value from stack
        lua_pop( lua, 1 );
        if( !result ) {
            break;
        }
    }
    m_buff->write( SERIALIZER_TABLE_END );
    return result;
}

bool TableSerializer::serialize_item( lua_State* lua, int idx ) {
    bool result{ true };
    int type = lua_type( lua, idx );
    if( lua_isnil( lua, idx ) ) {
        m_buff->write( static_cast<type_id_t>( TYPE_ID::NIL ) );
    }
    else if( lua_isboolean( lua, idx ) ) {
        write_item( static_cast<bool>( lua_toboolean( lua, idx ) ) );
    }
    else if( lua_isinteger( lua, idx ) ) {
        write_item( static_cast<integer_t>( lua_tointeger( lua, idx ) ) );
    }
    else if( lua_isnumber( lua, idx ) && type == LUA_TNUMBER ) {
        write_item( static_cast<float_t>( lua_tonumber( lua, idx ) ) );
    }
    else if( lua_isstring( lua, idx ) && type == LUA_TSTRING ) {
        std::size_t length;
        const char* str = lua_tolstring( lua, idx, &length );
        write_string( str, length );
    }
    else if( lua_istable( lua, idx ) ) {
        m_buff->write( static_cast<type_id_t>( TYPE_ID::TABLE ) );
        result = serialize_table( lua );
    }
    else {
        // Does not support other types
        result = false;
    }
    return result;
}

bool TableSerializer::deserialize_table( lua_State* lua ) {
    if( !m_buff->can_read( sizeof( char ) ) ) {
        return false;
    }
    if( m_buff->read<char>() != SERIALIZER_TABLE_BEGIN ) {
        return false;
    }
    bool result{ true };
    // Can we check for table end mark?
    if( !m_buff->can_read( sizeof( char ) ) ) {
        return false;
    }
    lua_newtable( lua );
    while( m_buff->peek<char>() != SERIALIZER_TABLE_END ) {
        // Key
        if( !deserialize_item( lua ) ) {
            result = false;
            break;
        }
        // Value
        if( !deserialize_item( lua ) ) {
            // Remove Key
            lua_pop( lua, 1 );
            result = false;
            break;
        }
        lua_settable( lua, -3 );
        if( !m_buff->can_read( sizeof( char ) ) ) {
            result = false;
            break;
        }
    }
    // Read table end
    if( result && m_buff->can_read( sizeof( char ) ) ) {
        m_buff->read<char>();
    }
    else {
        // Remove invalid table
        lua_pop( lua, 1 );
    }
    return result;
}  // namespace lua

bool TableSerializer::deserialize_item( lua_State* lua ) {
    if( !m_buff->can_read( sizeof( type_id_t ) ) ) {
        return false;
    }
    TYPE_ID type = static_cast<TYPE_ID>( m_buff->read<type_id_t>() );
    bool result{ false };
    switch( type ) {
    case TYPE_ID::BOOL:
        if( m_buff->can_read( sizeof( bool ) ) ) {
            lua_pushboolean( lua, m_buff->read<bool>() );
            result = true;
        }
        break;
    case TYPE_ID::INTEGER:
        if( m_buff->can_read( sizeof( integer_t ) ) ) {
            lua_pushinteger( lua, m_buff->read<integer_t>() );
            result = true;
        }
        break;
    case TYPE_ID::FLOAT:
        if( m_buff->can_read( sizeof( float_t ) ) ) {
            lua_pushnumber( lua, m_buff->read<float_t>() );
            result = true;
        }
        break;
    case TYPE_ID::STRING:
        if( m_buff->can_read( sizeof( item_size_t ) ) ) {
            std::size_t length = m_buff->read<item_size_t>();
            const void* str;
            if( m_buff->can_read( length ) ) {
                m_buff->read( &str, length );
                lua_pushlstring( lua, reinterpret_cast<const char*>( str ), length );
                result = true;
            }
        }
        break;
    case TYPE_ID::TABLE:
        result = deserialize_table( lua );
        break;
    case TYPE_ID::NIL:
        lua_pushnil( lua );
        result = true;
        break;
    }
    return result;
}  // namespace lua

}  // namespace lua
