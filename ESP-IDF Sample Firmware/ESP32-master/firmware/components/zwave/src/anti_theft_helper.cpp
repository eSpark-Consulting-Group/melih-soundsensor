#include "anti_theft_helper.h"
#include "esp32/rom/md5_hash.h"

#include <algorithm>
#include <random>

#include "hub_core/Common.h"
#include "wifi_service2/WifiTypes.h"

namespace gateways {
namespace zwave {

constexpr uint8_t AntiTheftHintVersion = 1;
constexpr uint8_t AntiTheftHintSize = 10;

std::vector<uint8_t> generateHint() {
    constexpr auto data32Size = ( AntiTheftHintSize + sizeof( uint32_t ) - 1 ) / sizeof( uint32_t );
    union {
        uint32_t data32[data32Size];
        uint8_t data8[AntiTheftHintSize];
    };

    std::generate_n( data32, data32Size, std::mt19937( time( nullptr ) ) );
    data8[AntiTheftHintSize - 1] = AntiTheftHintVersion;  // version;

    return { data8, data8 + AntiTheftHintSize };
}

BytesOpt getMagicCode( const std::vector<uint8_t>& hint ) {
    static uint8_t result[16];
    // check size
    if( hint.size() != AntiTheftHintSize ) {
        return {};
    }

    // check version
    if( hint.back() != AntiTheftHintVersion ) {
        return {};
    }

    static struct MD5Context md5_context;
    MD5Init( &md5_context );

    MD5Update( &md5_context, reinterpret_cast<const unsigned char*>( hint.data() ), hint.size() );

    auto mac = ( network::wifi_srv2::MacType::get() )->toHexString();
    std::transform( mac.begin(), mac.end(), mac.begin(), []( unsigned char c ) { return std::tolower( c ); } );
    mac += core::serial();

    MD5Update( &md5_context, reinterpret_cast<const unsigned char*>( mac.c_str() ), mac.length() );

    const unsigned char salt[] = { 0x8A, 0x9D, 0x04, 0x8E, 0xC1, 0x52, 0xCB, 0xF9,
                                   0x75, 0xD8, 0x07, 0xEA, 0x63, 0x9F, 0x83, 0x3B };
    MD5Update( &md5_context, salt, sizeof( salt ) );

    MD5Final( result, &md5_context );

    return BytesOpt( { result, result + AntiTheftHintSize } );
}

}  // namespace zwave
}  // namespace gateways
