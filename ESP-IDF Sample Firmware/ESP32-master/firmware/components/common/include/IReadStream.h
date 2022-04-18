#pragma once

#include <cinttypes>
#include <iterator>
#include <cstring>
#include <string>
#include <memory>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <limits>
#include <array>
#include <tuple>
#include <chrono>

namespace common {

class IReadStream {
    enum class SerializationMarker : std::uint8_t { Item, End };

public:
    using UPtr = std::unique_ptr<IReadStream>;

    template <typename T>
    using EnableForTrivialTypes = typename std::
        enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value || std::is_enum<T>::value, void>::type;

    virtual ~IReadStream() = default;

    virtual int read( char* data, size_t dataSize ) = 0;

    virtual size_t size() const {
        return std::numeric_limits<size_t>::max();  // means endless stream
    }

    virtual bool reset( unsigned int offset ) {
        return false;
    }

public:
    template <typename ValueType, typename = typename IReadStream::EnableForTrivialTypes<ValueType>>
    inline bool read( ValueType& value ) {
        if( sizeof( ValueType ) > size() ) {
            return false;
        }
        return sizeof( ValueType ) == read( reinterpret_cast<char*>( &value ), sizeof( ValueType ) );
    }

    template <class CharT, class Traits, class Allocator>
    bool read( std::basic_string<CharT, Traits, Allocator>& value ) {
        std::uint32_t itemsCount;
        if( !read( itemsCount ) ) {
            return false;
        }
        value.clear();
        value.reserve( itemsCount );
        auto itr = this->begin<CharT>( itemsCount );
        const auto end = this->end<CharT>();
        for( ; itr != end; ++itr ) {
            value.push_back( *itr );
        }
        return value.length() == itemsCount;
    }

    template <typename T>
    bool read( std::list<T>& list ) {
        return readListLikeContainer( list );
    }

    template <typename ValueType,
              unsigned int ArrSize,
              typename = typename IReadStream::EnableForTrivialTypes<ValueType>>
    bool read( std::array<ValueType, ArrSize>& arr ) {
        constexpr const std::uint32_t numBytes = sizeof( ValueType ) * ArrSize;
        {
            std::uint32_t readedNumBytes;
            if( !read( readedNumBytes ) || numBytes != readedNumBytes ) {
                return false;
            }
        }
        return numBytes == read( reinterpret_cast<char*>( arr.data() ), numBytes );
    }

    template <typename ValueType>
    bool read( std::vector<ValueType>& vec ) {
        std::uint32_t itemsCount;
        if( !read( itemsCount ) ) {
            return false;
        }
        vec.clear();
        vec.reserve( itemsCount );
        auto itr = this->begin<ValueType>( itemsCount );
        const auto end = this->end<ValueType>();
        for( ; itr != end; ++itr ) {
            vec.emplace_back( *itr );
        }
        return vec.size() == itemsCount;
    }

    template <typename T>
    bool read( std::unique_ptr<T>& uptr ) {
        auto instanceUPtr = T::create( *this );
        if( instanceUPtr ) {
            uptr = std::move( instanceUPtr );
            return true;
        }
        else {
            uptr = nullptr;
            return false;
        }
    }

    template <typename Key, typename Value>
    inline bool read( std::unordered_map<Key, Value>& map ) {
        return readMapLikeContainer( map );
    }

    template <typename Key, typename Value>
    inline bool read( std::map<Key, Value>& map ) {
        return readMapLikeContainer( map );
    }

    template <typename Value>
    inline bool read( std::set<Value>& set ) {
        return readSetLikeContainer( set );
    }

    template <typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
    inline bool read( T& object ) {
        return object.read( *this );
    }

    template <typename T1, typename T2>
    inline bool read( std::pair<T1, T2>& value ) {
        return read( value.first ) && read( value.second );
    }

    template <typename... Ts>
    bool read( std::tuple<Ts...>& tpl ) {
        // Simplest check to avoiding wrong read/write
        constexpr const std::uint32_t numElements = sizeof...( Ts );
        std::uint32_t readedNumElements;
        if( !read( readedNumElements ) || numElements != readedNumElements ) {
            return false;
        }

        bool isSuccess{ true };
        std::apply( [&isSuccess, this]( Ts&... args ) { ( ( isSuccess = isSuccess && read( args ) ), ... ); }, tpl );

        return isSuccess;
    }

    template <typename ClockType, typename Duration>
    bool read( std::chrono::time_point<ClockType, Duration>& tp ) {
        std::uint64_t time;
        if( !read( time ) ) {
            return false;
        }

        tp = std::chrono::time_point<ClockType, Duration>( Duration( time ) );

        static_assert( sizeof( decltype( tp.time_since_epoch().count() ) ) <= sizeof( time ) );

        return true;
    }

    template <typename ValueType>
    class Iterator : public std::iterator<std::input_iterator_tag, ValueType> {
        static const int END_OF_ITERATIONS = -1;

        IReadStream& m_rstream;
        int m_itemsLeft;
        ValueType m_value;

    public:
        explicit Iterator( IReadStream& rstream, int itemsCount = END_OF_ITERATIONS )
        : m_rstream( rstream )
        , m_itemsLeft( itemsCount ) {
            ++( *this );
        }

        Iterator& operator++() {
            if( m_rstream.size() < sizeof( ValueType ) || m_itemsLeft <= 0 ) {
                m_itemsLeft = END_OF_ITERATIONS;
                return *this;
            }

            if( m_rstream.read( m_value ) ) {
                --m_itemsLeft;
            }
            else {
                // If we can't read from stream simply terminate iterations
                m_itemsLeft = END_OF_ITERATIONS;
            }
            return *this;
        }

        Iterator operator++( int ) {
            Iterator retval = *this;
            ++( *this );
            return retval;
        }

        bool operator==( const Iterator& other ) const {
            return m_itemsLeft == other.m_itemsLeft;
        }

        bool operator!=( const Iterator& other ) const {
            return !( *this == other );
        }

        const ValueType& operator*() const {
            return m_value;
        }
    };

    template <typename ValueType>
    Iterator<ValueType> begin( std::size_t itemsCount ) {
        if( size() < itemsCount * sizeof( ValueType ) ) {
            return end<ValueType>();
        }
        return Iterator<ValueType>( *this, itemsCount );
    }

    template <typename ValueType>
    Iterator<ValueType> end() {
        return Iterator<ValueType>( *this );
    }

private:
    template <typename Container>
    bool readListLikeContainer( Container& list ) {
        SerializationMarker marker;
        while( read( marker ) ) {
            if( SerializationMarker::End == marker ) {
                return true;
            }
            else if( SerializationMarker::Item != marker ) {
                return false;
            }

            auto itr = list.emplace( list.end() );
            if( !read( *itr ) ) {
                return false;
            }
        }
        return false;
    }

    template <typename Container>
    bool readSetLikeContainer( Container& set ) {
        SerializationMarker marker;
        while( read( marker ) ) {
            if( SerializationMarker::End == marker ) {
                return true;
            }
            else if( SerializationMarker::Item != marker ) {
                return false;
            }

            typename Container::key_type val;
            if( !read( val ) ) {
                return false;
            }
            set.insert( std::move( val ) );
        }
        return false;
    }

    template <typename Container>
    bool readMapLikeContainer( Container& map ) {
        SerializationMarker marker;
        while( read( marker ) ) {
            if( SerializationMarker::End == marker ) {
                return true;
            }
            else if( SerializationMarker::Item != marker ) {
                return false;
            }

            typename Container::key_type key;
            typename Container::mapped_type value;
            if( !read( key ) || !read( value ) ) {
                return false;
            }

            map.emplace( std::move( key ), std::move( value ) );
        }
        return false;
    }
};

class ArrayRStream final : public IReadStream {
    const char* m_buf = nullptr;
    std::size_t m_bufsize = 0;
    std::size_t m_rpos = 0;

public:
    ArrayRStream() = default;

    ArrayRStream( const ArrayRStream& ) = default;

    ArrayRStream( const char* buf, std::size_t bufsize )
    : m_buf( buf )
    , m_bufsize( bufsize ) {}

    using IReadStream::read;

    int read( char* data, std::size_t dataSize ) override {
        std::size_t sizeToRead = std::min( dataSize, size() );
        std::memcpy( data, m_buf + m_rpos, sizeToRead );
        m_rpos += sizeToRead;
        return sizeToRead;
    }

    inline std::size_t size() const override {
        return m_bufsize - m_rpos;
    }

    ArrayRStream& operator=( const ArrayRStream& stream ) = default;
};

}  // namespace common
