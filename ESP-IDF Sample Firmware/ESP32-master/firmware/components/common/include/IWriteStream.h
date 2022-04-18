
#pragma once

#include "IValue.h"

#include <cstddef>
#include <type_traits>
#include <string>
#include <cstring>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>
#include <vector>
#include <array>
#include <tuple>
#include <chrono>

namespace common {

class IWriteStream {
    enum class SerializationMarker : std::uint8_t { Item, End };

public:
    using UPtr = std::unique_ptr<IWriteStream>;

    template <typename T>
    using EnableForTrivialTypes = typename std::
        enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value || std::is_enum<T>::value, void>::type;

    virtual ~IWriteStream() = default;

    virtual int write( const char* buffer, std::size_t size ) = 0;

    virtual bool flush() = 0;

    virtual bool close() = 0;

    template <typename ValueType, typename = typename IWriteStream::EnableForTrivialTypes<ValueType>>
    inline bool write( const ValueType& value ) {
        return sizeof( ValueType ) == write( reinterpret_cast<const char*>( &value ), sizeof( ValueType ) );
    }

    template <typename CharT, typename Traits, typename Allocator>
    inline bool write( const std::basic_string<CharT, Traits, Allocator>& str ) {
        std::uint32_t lenght = str.length();
        return write( lenght ) && lenght == write( str.c_str(), lenght );
    }

    template <typename ValueType,
              unsigned int ArrSize,
              typename = typename IWriteStream::EnableForTrivialTypes<ValueType>>
    bool write( const std::array<ValueType, ArrSize>& arr ) {
        constexpr const std::uint32_t numBytes = sizeof( ValueType ) * ArrSize;
        return write( numBytes ) && numBytes == write( reinterpret_cast<const char*>( arr.data() ), numBytes );
    }

    template <typename T>
    inline bool write( const std::list<T>& list ) {
        return writeListLikeContainer( list );
    }

    template <typename Key, typename Value>
    inline bool write( const std::unordered_map<Key, Value>& map ) {
        return writeMapLikeContainer( map );
    }

    template <typename Key, typename Value>
    inline bool write( const std::map<Key, Value>& map ) {
        return writeMapLikeContainer( map );
    }

    template <typename Key>
    inline bool write( const std::set<Key>& set ) {
        return writeListLikeContainer( set );
    }

    template <typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
    inline bool write( const T& object ) {
        return object.write( *this );
    }

    template <typename T>
    bool write( const std::unique_ptr<T>& uptr ) {
        return uptr ? uptr->write( *this ) : false;
    }

    template <typename T1, typename T2>
    inline bool write( const std::pair<T1, T2>& value ) {
        return write( value.first ) && write( value.second );
    }

    template <typename... Ts>
    bool write( const std::tuple<Ts...>& tpl ) {
        // Simplest check to avoiding wrong read/write
        constexpr const std::uint32_t numElements = sizeof...( Ts );
        if( !write( numElements ) ) {
            return false;
        }

        bool isSuccess{ true };
        std::apply( [&isSuccess, this]( const Ts&... args ) { ( ( isSuccess = isSuccess && write( args ) ), ... ); },
                    tpl );

        return isSuccess;
    }

    template <typename ClockType, typename Duration>
    bool write( const std::chrono::time_point<ClockType, Duration>& tp ) {
        const std::uint64_t time = tp.time_since_epoch().count();

        static_assert( sizeof( decltype( tp.time_since_epoch().count() ) ) <= sizeof( time ) );

        return write( time );
    }

private:
    template <typename Container>
    bool writeListLikeContainer( const Container& list ) {
        auto end = list.cend();
        for( auto itr = list.cbegin(); itr != end; ++itr ) {
            if( !write( SerializationMarker::Item ) ) {
                return false;
            }
            if( !write( *itr ) ) {
                return false;
            }
        }
        return write( SerializationMarker::End );
    }

    template <typename Container>
    bool writeMapLikeContainer( const Container& map ) {
        auto end = map.cend();
        for( auto itr = map.cbegin(); itr != end; ++itr ) {
            if( !write( SerializationMarker::Item ) ) {
                return false;
            }
            if( !write( itr->first ) || !write( itr->second ) ) {
                return false;
            }
        }
        return write( SerializationMarker::End );
    }
};

class VectorWStream final : public IWriteStream {
public:
    std::vector<char> m_buf;

    VectorWStream( std::size_t reservedSize = 512 ) {
        m_buf.reserve( reservedSize );
    }

    VectorWStream( std::vector<char>&& buf )
    : m_buf( std::move( buf ) ) {}

    using IWriteStream::write;

    int write( const char* data, std::size_t dataSize ) override;

    inline bool flush() override {
        m_buf.clear();
        return true;
    }

    inline bool close() override {
        return flush();
    }

    inline std::size_t dataSize() const {
        return m_buf.size();
    }

    inline std::size_t freeSpace() const {
        return 0x10000;
    }

    inline auto& buffer() {
        return m_buf;
    }

    inline const auto& buffer() const {
        return m_buf;
    }

    std::string toString() const {
        return std::string( buffer().data(), dataSize() );
    }
};

}  // namespace common
