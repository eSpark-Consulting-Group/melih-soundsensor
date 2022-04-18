#pragma once

#include <vector>
#include <cstdint>
#include <utility>
#include <cstring>
#include <algorithm>
#include <type_traits>

namespace common {

class SerializationBuffer {
public:
    static constexpr std::size_t DEFAULT_BUFFER_LENGTH = 512u;
    typedef std::size_t position_t;

    void init( std::size_t buffer_length = DEFAULT_BUFFER_LENGTH ) {
        m_buff = std::vector<char>( buffer_length, 0 );
        m_position = 0u;
    }

    void init( std::vector<char>&& data ) {
        m_buff = std::move( data );
        m_position = 0;
    }

    position_t getPosition() const {
        return m_position;
    }

    void restorePosition( position_t position ) {
        m_position = position;
    }

    bool can_read( std::size_t length ) {
        return m_position + length <= m_buff.size();
    }

    template <typename T,
              typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type = 0>
    void write( T val ) {
        constexpr std::size_t length = sizeof( T );
        write_chunk( reinterpret_cast<const char*>( &val ), length );
    }

    void write( const void* buff, std::size_t length ) {
        write_chunk( reinterpret_cast<const char*>( buff ), length );
    }

    template <typename T,
              typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type = 0>
    T peek() {
        constexpr std::size_t length = sizeof( T );
        const char* buff;
        peek_data( &buff );
        T val;
        std::memcpy( &val, buff, length );
        return val;
    }

    template <typename T,
              typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type = 0>
    T read() {
        constexpr std::size_t length = sizeof( T );
        T val = peek<T>();
        m_position += length;
        return val;
    }

    void read( const void** buff, std::size_t length ) {
        peek_data( reinterpret_cast<const char**>( buff ) );
        m_position += length;
    }

    std::vector<char> extract() {
        m_buff.resize( m_position );
        m_position = 0u;
        return std::move( m_buff );
    }

private:
    void reserve_space( std::size_t len ) {
        const std::size_t required_size = m_position + len;
        if( required_size > m_buff.size() ) {
            std::size_t new_size = ( required_size / DEFAULT_BUFFER_LENGTH + 1 ) * DEFAULT_BUFFER_LENGTH;
            m_buff.resize( new_size );
        }
    }

    void write_chunk( const char* chunk, std::size_t length ) {
        reserve_space( length );
        std::copy_n( chunk, length, m_buff.begin() + m_position );
        m_position += length;
    }

    void peek_data( const char** data ) {
        *data = static_cast<const char*>( m_buff.data() ) + m_position;
    }

private:
    std::vector<char> m_buff;
    position_t m_position = 0u;
};

}  // namespace common
