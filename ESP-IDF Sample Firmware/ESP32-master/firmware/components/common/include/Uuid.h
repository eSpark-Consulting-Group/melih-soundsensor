#pragma once

#include "IReadStream.h"
#include "IWriteStream.h"

#include <string>
#include <functional>
#include <optional>
#include <list>
#include <set>

namespace common {

class Uuid {
public:
    using UuidUInt = std::uint64_t;
    union UuidType {
        char uuid_bytes[8];
        std::uint32_t uuid_words[2];
        std::uint64_t uuid64;
        UuidUInt uuid;
    };

    static Uuid generate();

    Uuid() = default;

    Uuid( const Uuid& ) = default;

    inline Uuid( const UuidUInt& uuid ) {
        m_uuid.uuid = uuid;
    }

    static std::optional<Uuid> createFromHexStr( const std::string& str );

    static std::optional<Uuid> createFromHexStr( const char* str ) {
        return str ? createFromHexStr( std::string{ str } ) : std::nullopt;
    }

    inline operator UuidUInt() const {
        return m_uuid.uuid64;
    }

    inline operator bool() const {
        return 0 != m_uuid.uuid;
    }

    inline bool operator<( const Uuid& uuid ) const {
        return m_uuid.uuid64 < uuid.m_uuid.uuid64;
    }

    inline bool operator==( const Uuid& uuid ) const {
        return m_uuid.uuid64 == uuid.m_uuid.uuid64;
    }

    /* #TODO: I think we should to implement this for simplicity of
    using Uuid as key in KeyValueStorage
    operator std::string() const {
    }*/

    std::string toHexString( const char* prefix = "" ) const;

    inline bool write( IWriteStream& stream ) const {
        return stream.write( m_uuid.uuid_words[0] ) && stream.write( m_uuid.uuid_words[1] );
    }

    inline bool read( IReadStream& stream ) {
        return stream.read( m_uuid.uuid_words[0] ) && stream.read( m_uuid.uuid_words[1] );
    }

protected:
    UuidType m_uuid{ 0 };
};

}  // namespace common

namespace std {
template <>
class hash<common::Uuid> {
    std::hash<std::uint64_t> m_hash;

public:
    size_t operator()( const common::Uuid& uuid ) const {
        return m_hash( std::uint64_t( uuid ) );
    }
};

}  // end of namespace std

namespace common {

using UuidList = std::list<Uuid>;
using UuidSet = std::set<Uuid>;

}  // namespace common
