#pragma once

#include <lua.hpp>
#include <vector>
#include <memory>
#include <SerializationBuffer.h>

namespace lua {

class TableSerializer {
public:
    typedef std::vector<char> SerializedParams;

    SerializedParams serialize( lua_State* lua );
    bool deserialize( lua_State* lua, SerializedParams buff );

private:
    template <typename T,
              typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type = 0>
    void write_item( T item );
    void write_string( const void* str, std::size_t length );
    bool serialize_table( lua_State* lua );
    bool serialize_item( lua_State* lua, int idx );
    bool deserialize_table( lua_State* lua );
    bool deserialize_item( lua_State* lua );

private:
    std::unique_ptr<common::SerializationBuffer> m_buff;
};

}  // namespace lua
