#include "Uuid.h"

#include "HardUtils.h"

namespace common {

Uuid Uuid::generate() {
    Uuid uuid;
    uuid.m_uuid.uuid_words[0] = hard::common::generateUniqueId();
    uuid.m_uuid.uuid_words[1] = hard::common::generateUniqueId();
    return uuid;
}

std::optional<Uuid> Uuid::createFromHexStr( const std::string& str ) {
    auto pos = str.find( '_' );
    if( pos == str.length() - 1 ) {
        return std::nullopt;
    }
    char* end;
    const auto val = strtoull( str.substr( pos + 1 ).c_str(), &end, 16 );
    return std::make_optional( Uuid( val ) );
}

std::string Uuid::toHexString( const char* prefix ) const {
    char buf[32];
    snprintf( buf, sizeof( buf ), "%s%.16llX", prefix, m_uuid.uuid64 );
    return std::string{ buf };
}

}  // namespace common
