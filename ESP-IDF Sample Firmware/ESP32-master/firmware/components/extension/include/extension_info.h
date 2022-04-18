#pragma once

#include <cstdint>
#include <string>
#include <sstream>

namespace extension {

using ExtensionID = std::string;

struct Version {
    std::uint8_t major;
    std::uint8_t minor;

    inline bool operator==( const Version& rhs ) const {
        return ( major == rhs.major ) && ( minor == rhs.minor );
    }

    inline bool operator!=( const Version& rhs ) const {
        return !( *this == rhs );
    }

    auto toString() const {
        std::stringstream ss;
        ss << std::size_t( major ) << "." << std::size_t( minor );
        return ss.str();
    }
};

template <typename ID_T, typename VER_T>
struct ExtensionInfo {
    ID_T id;
    VER_T version;

    inline bool operator==( const ExtensionInfo& rhs ) const {
        return ( id == rhs.id ) && ( version == rhs.version );
    }

    inline bool operator!=( const ExtensionInfo& rhs ) const {
        return !( *this == rhs );
    }
};

}  // namespace extension
