#pragma once

#include "IValue.h"

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <iterator>
#include <tuple>
#include <cassert>
#include <ostream>

namespace common {
class IReadStream;
}

namespace storage {

class KeyValueStorage {
public:
    typedef std::vector<char> BlobType;

    struct ValueT {
        enum Types : uint8_t { INT_8, INT_16, INT_32, INT_64, BLOB, NONE_TYPE };
        union {
            std::int8_t int8;
            std::int16_t int16;
            std::int32_t int32;
            std::int64_t int64;
            BlobType blob;
        };
        Types val_type;

        ValueT()
        : val_type( NONE_TYPE ) {}

        ValueT( const ValueT& _val )
        : val_type( _val.val_type ) {
            switch( _val.val_type ) {
            case INT_8:
                int8 = _val.int8;
                break;
            case INT_16:
                int16 = _val.int16;
                break;
            case INT_32:
                int32 = _val.int32;
                break;
            case INT_64:
                int64 = _val.int64;
                break;
            case BLOB:
                new( &blob ) BlobType;
                blob = _val.blob;
                break;
            default:
                break;
            }
        }

        ~ValueT() {
            _clear();
        };

        template <typename T>
        ValueT& operator=( const T& _val );

        ValueT& operator=( const ValueT& _val ) {
            _clear();
            val_type = _val.val_type;
            switch( _val.val_type ) {
            case INT_8:
                int8 = _val.int8;
                break;
            case INT_16:
                int16 = _val.int16;
                break;
            case INT_32:
                int32 = _val.int32;
                break;
            case INT_64:
                int64 = _val.int64;
                break;
            case BLOB:
                new( &blob ) BlobType;
                blob = _val.blob;
                break;
            default:
                break;
            }

            return *this;
        };

        bool operator==( const ValueT& _val ) const {
            if( val_type != _val.val_type )
                return false;
            switch( _val.val_type ) {
            case INT_8:
                return int8 == _val.int8;
            case INT_16:
                return int16 == _val.int16;
            case INT_32:
                return int32 == _val.int32;
            case INT_64:
                return int64 == _val.int64;
            case BLOB:
                return blob == _val.blob;
            default:
                return false;
            }
        }

        inline bool operator!=( const ValueT& _val ) const {
            return !( *this == _val );
        }

        template <typename T>
        bool is_type() const {
            return false;
        }

        template <typename T>
        T& get();

        bool isEmpty() const {
            return NONE_TYPE == val_type;
        }

        inline void _clear() {
            if( BLOB == val_type )
                blob.~vector();
        };

        const char* typeName() const;
    };

    class ConstIterator : public std::iterator<std::input_iterator_tag,  // iterator_category
                                               std::string> {            // value_type
    public:
        using ValueType = ValueT;

        struct IteratorImpl {
            typedef std::unique_ptr<IteratorImpl> uptr;

            virtual ~IteratorImpl() = default;
            virtual uptr clone() const = 0;
            virtual bool next() = 0;
            virtual uint32_t key_hash() = 0;
            virtual const std::string key() const = 0;
            virtual const ValueType value() const = 0;
        };

        using itr_impl_p = IteratorImpl::uptr;

        ConstIterator() = default;

        ConstIterator( const ConstIterator& _itr )
        : m_impl( _itr.m_impl->clone() ) {}

        ConstIterator( ConstIterator&& ) = default;

        ConstIterator( itr_impl_p _impl )
        : m_impl( std::move( _impl ) ) {
            assert( m_impl );
        }

        ConstIterator& operator=( const ConstIterator& _itr ) {
            m_impl = _itr.m_impl ? _itr.m_impl->clone() : nullptr;
            return *this;
        };

        ConstIterator& operator=( ConstIterator&& ) = default;

        ConstIterator& operator++() {
            if( !m_impl->next() )
                m_impl.reset();
            return *this;
        };

        ConstIterator operator++( int ) {  // postfix variant
            if( !m_impl )
                return ConstIterator();

            ConstIterator current( m_impl->clone() );
            if( !m_impl->next() )
                m_impl.reset();
            return std::move( current );
        };

        bool operator==( const ConstIterator& _other ) const {
            if( !m_impl )
                return !_other.m_impl;
            else if( !_other.m_impl )
                return !m_impl;
            else
                return m_impl->key_hash() == _other.m_impl->key_hash() && m_impl->key() == _other.m_impl->key();
        }

        bool operator!=( const ConstIterator& _other ) const {
            return !( *this == _other );
        }

        const std::string operator*() const {
            return m_impl->key();
        }

        const ValueType value() const {
            return m_impl->value();
        }

    private:
        itr_impl_p m_impl;
    };

    virtual ~KeyValueStorage() = default;

    virtual bool getInt8( const std::string& key, std::int8_t& value ) = 0;
    virtual bool getInt16( const std::string& key, std::int16_t& value ) = 0;
    virtual bool getInt32( const std::string& key, std::int32_t& value ) = 0;
    virtual bool getInt64( const std::string& key, std::int64_t& value ) = 0;
    virtual bool getBlob( const std::string& key, BlobType& value ) = 0;
    virtual bool setInt8( const std::string& key, std::int8_t value ) = 0;
    virtual bool setInt16( const std::string& key, std::int16_t value ) = 0;
    virtual bool setInt32( const std::string& key, std::int32_t value ) = 0;
    virtual bool setInt64( const std::string& key, std::int64_t value ) = 0;
    virtual bool setBlob( const std::string& key, const BlobType& value ) = 0;
    virtual bool setBlob( const std::string& key, common::IReadStream& rstream, const size_t len ) = 0;

    common::IValue::UPtr getIValue( const std::string& key );

    bool set( const std::string& key, const common::IValue& value );

    inline bool set( const std::string& key, const common::IValue::UPtr& value ) {
        return value ? set( key, *value ) : false;
    }

    virtual bool exists( const std::string& key ) = 0;
    virtual bool remove( const std::string& key ) = 0;
    virtual bool removeAll() = 0;

    virtual ConstIterator cend() const {
        return ConstIterator();
    };
    virtual ConstIterator cbegin() const {
        return ConstIterator();
    };
    virtual ConstIterator end() const {
        return cend();
    };
    virtual ConstIterator begin() const {
        return cbegin();
    };

    void printInfo() const;
};

template <>
inline bool KeyValueStorage::ValueT::is_type<std::int8_t>() const {
    return INT_8 == val_type;
};

template <>
inline bool KeyValueStorage::ValueT::is_type<std::int16_t>() const {
    return INT_16 == val_type;
};

template <>
inline bool KeyValueStorage::ValueT::is_type<std::int32_t>() const {
    return INT_32 == val_type;
};

template <>
inline bool KeyValueStorage::ValueT::is_type<std::int64_t>() const {
    return INT_64 == val_type;
};

template <>
inline bool KeyValueStorage::ValueT::is_type<KeyValueStorage::BlobType>() const {
    return BLOB == val_type;
};

template <>
inline std::int8_t& KeyValueStorage::ValueT::get<std::int8_t>() {
    return int8;
};

template <>
inline std::int16_t& KeyValueStorage::ValueT::get<std::int16_t>() {
    return int16;
};

template <>
inline std::int32_t& KeyValueStorage::ValueT::get<std::int32_t>() {
    return int32;
};

template <>
inline std::int64_t& KeyValueStorage::ValueT::get<std::int64_t>() {
    return int64;
};

template <>
inline KeyValueStorage::BlobType& KeyValueStorage::ValueT::get<KeyValueStorage::BlobType>() {
    return blob;
};

template <>
inline KeyValueStorage::ValueT& KeyValueStorage::ValueT::operator=<std::int8_t>( const std::int8_t& _val ) {
    _clear();
    val_type = INT_8;
    int8 = _val;
    return *this;
}

template <>
inline KeyValueStorage::ValueT& KeyValueStorage::ValueT::operator=<std::int16_t>( const std::int16_t& _val ) {
    _clear();
    val_type = INT_16;
    int16 = _val;
    return *this;
}

template <>
inline KeyValueStorage::ValueT& KeyValueStorage::ValueT::operator=<std::int32_t>( const std::int32_t& _val ) {
    _clear();
    val_type = INT_32;
    int32 = _val;
    return *this;
}

template <>
inline KeyValueStorage::ValueT& KeyValueStorage::ValueT::operator=<std::int64_t>( const std::int64_t& _val ) {
    _clear();
    val_type = INT_64;
    int64 = _val;
    return *this;
}

template <>
inline KeyValueStorage::ValueT& KeyValueStorage::ValueT::operator=
    <KeyValueStorage::BlobType>( const KeyValueStorage::BlobType& _val ) {
    if( BLOB != val_type )
        new( &blob ) BlobType;
    val_type = BLOB;
    blob = _val;
    return *this;
}

inline std::ostream& operator<<( std::ostream& os, const KeyValueStorage::ValueT& _val ) {
    os << "type: ";
    switch( _val.val_type ) {
    case KeyValueStorage::ValueT::INT_8:
        os << "int8_t(" << (int)_val.int8 << ")";
        break;
    case KeyValueStorage::ValueT::INT_16:
        os << "int16_t(" << (int)_val.int16 << ")";
        break;
    case KeyValueStorage::ValueT::INT_32:
        os << "int32_t(" << (int)_val.int32 << ")";
        break;
    case KeyValueStorage::ValueT::INT_64:
        os << "int64_t(" << (long int)_val.int64 << ")";
        break;
    case KeyValueStorage::ValueT::BLOB:
        os << "blob(" << _val.blob.size() << " bytes)";
        break;
    case KeyValueStorage::ValueT::NONE_TYPE:
        os << "none";
        break;
    default:
        os << "nothing good(";
        break;
    }

    return os;
}

}  // namespace storage
