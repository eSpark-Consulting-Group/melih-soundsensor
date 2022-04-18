#pragma once

#include <vector>
#include <znet/optional.hpp>
#include <string>

namespace gateways {
namespace zwave {

using Bytes = std::vector<uint8_t>;
using BytesOpt = znet::utils::optional<Bytes>;

Bytes generateHint();

/*
BytesOpt getMagicCode( const unsigned char* hint,
                       uint8_t hint_size,
                       const unsigned char* userSpecificString,
                       uint8_t hint_size );
*/
BytesOpt getMagicCode( const std::vector<uint8_t>& hint );

// BytesOpt getMagicCode( const unsigned char* hint, uint8_t hint_size );

}  // namespace zwave
}  // namespace gateways
