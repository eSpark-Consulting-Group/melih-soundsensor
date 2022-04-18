#pragma once

#include <stdint.h>
#include <assert.h>

namespace common {
template <class T, uint32_t N>
class Array {
public:
    typedef T value_type;

    Array() {
        static_assert( N != 0, "total length of array shall be non zero" );
        m_buff = new T[N];
    }

    ~Array() {
        delete[] m_buff;
    }

    Array( const Array& other ) = delete;
    Array( const Array&& other ) = delete;
    Array& operator=( const Array& other ) = delete;

    inline uint32_t capacity() const {
        return N;
    }

    inline uint32_t size() const {
        return m_count;
    }

    bool resize( uint32_t newSize );

    bool copyFrom( const T* buff, uint32_t n );

    bool pushBack( const T& node );
    bool eraseAndShift( uint32_t i );
    bool eraseAndSwap( uint32_t i );

    const T& operator[]( uint32_t i ) const;
    T& operator[]( uint32_t i );

    const T& at( uint32_t i ) const;
    T& at( uint32_t i );

    template <class Process>
    void foreach( Process process );

    template <class Condition>
    T* find( Condition condition );

    template <class Condition>
    const T* find( Condition condition ) const;

    template <class Condition>
    T* findNext( const T* origin, Condition condition );

    template <class Condition>
    const T* findNext( const T* origin, Condition condition ) const;

    const T* begin() const {
        return m_buff;
    }
    T* begin() {
        return m_buff;
    }

    const T* end() const {
        return m_buff + m_count;
    }
    T* end() {
        return m_buff + m_count;
    }

    const uint8_t* data() const {
        return reinterpret_cast<const uint8_t*>( m_buff );
    }

    uint8_t* data() {
        return reinterpret_cast<uint8_t*>( m_buff );
    }

private:
    uint32_t m_count = 0;
    T* m_buff = nullptr;
};

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
inline bool Array<T, N>::resize( uint32_t newSize ) {
    if( newSize > N ) {
        return false;
    }

    if( newSize > m_count ) {
        while( m_count < newSize ) {
            new( &m_buff[m_count++] ) T();
        }
    }
    else {
        while( m_count > newSize ) {
            m_buff[--m_count].~T();
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
inline bool Array<T, N>::copyFrom( const T* buff, uint32_t n ) {
    if( n == 0 || ( m_count + n ) > N ) {
        return false;
    }

    uint32_t k = 0;

    while( k < n ) {
        new( &m_buff[m_count++] ) T( buff[k++] );
    }

    return true;
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
inline bool Array<T, N>::pushBack( const T& node ) {
    if( m_count >= N ) {
        return false;
    }

    new( &m_buff[m_count++] ) T( node );

    return true;
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
inline bool Array<T, N>::eraseAndShift( uint32_t i ) {
    if( i >= m_count ) {
        return false;
    }

    m_buff[i].~T();

    if( m_count == 1 ) {
        m_count = 0;
    }
    else {
        memmove( &m_buff[i], &m_buff[i + 1], ( m_count - i - 1 ) * sizeof( T ) );

        --m_count;
    }

    return true;
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
inline bool Array<T, N>::eraseAndSwap( uint32_t i ) {
    if( m_count <= i ) {
        return false;
    }

    m_buff[i].~T();

    if( m_count == 1 ) {
        m_count = 0;
    }
    else {
        memcpy( &m_buff[i], &m_buff[--m_count], sizeof( T ) );
    }

    return true;
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
const T& Array<T, N>::operator[]( uint32_t i ) const {
    assert( i < m_count );
    return m_buff[i];
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
T& Array<T, N>::operator[]( uint32_t i ) {
    assert( i < m_count );
    return m_buff[i];
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
const T& Array<T, N>::at( uint32_t i ) const {
    assert( i < m_count );
    return m_buff[i];
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
T& Array<T, N>::at( uint32_t i ) {
    assert( i < m_count );
    return m_buff[i];
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
template <class Process>
inline void Array<T, N>::foreach( Process process ) {
    for( auto i = 0; i < m_count; ++i ) {
        process( m_buff[i] );
    }
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
template <class Condition>
inline T* Array<T, N>::find( Condition condition ) {
    T* result = end();

    auto ptr = m_buff;

    for( auto i = 0; i < m_count; ++i, ++ptr ) {
        if( condition( *ptr ) ) {
            result = ptr;
            break;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
template <class Condition>
inline const T* Array<T, N>::find( Condition condition ) const {
    const T* result = end();

    auto ptr = m_buff;

    for( auto i = 0; i < m_count; ++i, ++ptr ) {
        if( condition( *ptr ) ) {
            result = ptr;
            break;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
template <class Condition>
inline T* Array<T, N>::findNext( const T* origin, Condition condition ) {
    T* result = end();

    auto i = origin - m_buff;

    assert( ( m_buff + i ) == origin );

    if( i >= m_count ) {
        return result;
    }

    auto ptr = &m_buff[i];

    for( /* EMPTY */; i < m_count; ++i, ++ptr ) {
        if( condition( *ptr ) ) {
            result = ptr;
            break;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------

template <class T, uint32_t N>
template <class Condition>
inline const T* Array<T, N>::findNext( const T* origin, Condition condition ) const {
    const T* result = end();

    auto i = origin - m_buff;

    assert( ( m_buff + i ) == origin );

    if( i >= m_count ) {
        return result;
    }

    auto ptr = &m_buff[i];

    for( /* EMPTY */; i < m_count; ++i, ++ptr ) {
        if( condition( *ptr ) ) {
            result = ptr;
            break;
        }
    }

    return result;
}

};  // namespace common
