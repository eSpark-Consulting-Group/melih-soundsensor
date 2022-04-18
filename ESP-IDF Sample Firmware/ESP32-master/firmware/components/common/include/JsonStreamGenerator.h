
#pragma once

#include "KeyValueStorage.h"

#include <cstddef>
#include <cinttypes>
#include <string>
#include <list>
#include <set>
#include <unordered_set>

namespace gason {
class JsonValue;
}  // namespace gason

namespace storage {
class KeyValueStorage;
class BlobType;
}  // namespace storage

namespace common {

class IWriteStream;
class IReadStream;
class IValue;

class JsonStreamGenerator final {
    enum class State { Root, ValuePart, FirstItem, Item };

public:
    JsonStreamGenerator( IWriteStream& outputStream );

    ~JsonStreamGenerator();

public:
    bool writePropertyKey( const char* key, std::uint16_t keySize );

    bool writePropertyKey( const char* key );

    inline bool writePropertyKey( const std::string& key ) {
        return writePropertyKey( key.c_str() );
    }

    bool writePropertyValue( const char* value, std::uint16_t valueSize );

    bool writePropertyValue( const char* value );

    bool writeProperty( const char* key, std::uint16_t keySize, const char* value, std::uint16_t valueSize );

    bool writeProperty( const char* key, const char* value, std::uint16_t valueSize );

    bool writeProperty( const char* key, const char* value );

    template <class CharT, class Traits, class Allocator>
    inline bool writeProperty( const char* key, const std::basic_string<CharT, Traits, Allocator>& value ) {
        return writeProperty( key, value.c_str(), value.size() );
    }

    bool writeProperty( const char* key, std::uint16_t keySize, float value );

    bool writeProperty( const char* key, std::uint16_t keySize, std::uint16_t value );

    bool writeProperty( const char* key, std::uint16_t keySize, std::uint32_t value );

    bool writeProperty( const char* key, std::uint16_t keySize, std::int16_t value );

    bool writeProperty( const char* key, std::uint16_t keySize, std::int32_t value );

    bool writeProperty( const char* key, std::uint16_t keySize, std::uint64_t value );

    bool writeProperty( const char* key, std::uint16_t keySize, std::int64_t value );

    bool writeProperty( const char* key, float value );

    bool writeProperty( const char* key, std::uint16_t value );

    bool writeProperty( const char* key, std::uint32_t value );

    bool writeProperty( const char* key, std::int16_t value );

    bool writeProperty( const char* key, std::int32_t value );

    bool writeProperty( const char* key, std::uint64_t value );

    bool writeProperty( const char* key, std::int64_t value );

    bool writeProperty( const char* key, std::uint16_t keySize, bool value );

    bool writeProperty( const char* key, bool value );

    bool writeProperty( const char* key, const IValue& value );

    bool writeProperty( const char* key, const IValue* value );

    bool writeProperty( const char* key, const gason::JsonValue& value );

    bool writeProperty( const char* key, const std::nullptr_t ) {
        return writePropertyKey( key ) && writeNullValue() && writeItemDelimiter();
    }

    template <typename _Type, typename _Alloc>
    bool writeProperty( const char* key, const std::vector<_Type, _Alloc>& list ) {
        return writeListLikeContainer( key, list );
    }

    template <typename _Type, typename _Alloc>
    bool writeProperty( const char* key, const std::list<_Type, _Alloc>& list ) {
        return writeListLikeContainer( key, list );
    }

    template <typename _Key, typename _Compare, typename _Alloc>
    bool writeProperty( const char* key, const std::set<_Key, _Compare, _Alloc>& set ) {
        return writeListLikeContainer( key, set );
    }

    template <typename _Value, typename _Hash, typename _Pred, typename _Alloc>
    bool writeProperty( const char* key, const std::unordered_set<_Value, _Hash, _Pred, _Alloc>& set ) {
        return writeListLikeContainer( key, set );
    }

    template <typename ElemT, size_t ArrayLen>
    bool writeProperty( const char* key, const std::array<ElemT, ArrayLen>& array ) {
        return writeListLikeContainer( key, array );
    }

    void setFirstItemState() {
        m_state = State::FirstItem;
    }

    void setDept( int dept ) {
        m_depth = dept;
    }

    bool writeRawProperty( const char* key, const char* value );

    bool writeRawProperty( const char* key, IReadStream& stream );

    bool writeRawProperty( const char* key, const storage::KeyValueStorage::BlobType& blob );

    bool writeOpeningObject();

    bool writeOpeningArray();

    bool writeClosingSymbol();

    bool writeJsonEnd();

    bool writePropertyDelimiter();

    bool writeItemDelimiter();

    bool writeNullValue();

    bool writeEmptyObject();

    bool writeEmptyArray();

private:
    template <typename Container>
    bool writeListLikeContainer( const char* key, const Container& container ) {
        if( !writePropertyKey( key ) || !writeOpeningArray() ) {
            return false;
        }

        for( const auto& value : container ) {
            if( !writeProperty( nullptr, value ) ) {
                return false;
            }
        }

        return writeClosingSymbol();
    }

    bool write( char symbol );

    bool write( const char* property, int size );

    bool write( const char* property );

    bool writeStringProperty( const char* property, std::uint16_t size );

    bool checkAndWriteOpeningBrackets();

    bool checkAndWriteComma();

private:
    bool incrementDepth();

    bool decrementDepth();

private:
    constexpr static int8_t MaxDepthSize = 32;

private:
    IWriteStream& m_outputStream;

    State m_state = State::Root;

    int m_depth = 0;
    int m_bracketsStack = 0;
    bool m_isOk = true;
};

}  // namespace common
