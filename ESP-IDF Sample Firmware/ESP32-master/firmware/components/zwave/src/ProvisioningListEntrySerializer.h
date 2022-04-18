#pragma once

#include <string>
#include <memory>

#include "SerializationBuffer.h"

namespace znet {
struct ProvisioningListEntry;
}

namespace gateways {
namespace zwave {

class ProvisioningListEntrySerializer {
public:
    typedef std::vector<char> SerializedParams;

    SerializedParams serialize( const znet::ProvisioningListEntry& entry );
    bool deserialize( SerializedParams&& buff, znet::ProvisioningListEntry& entry );

private:
    using type_id_t = char;
    using item_size_t = std::uint16_t;
    enum class TYPE_ID : type_id_t { INTEGER = 0u, STRING, BLOB, NIL };

    template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    void write_item( T item );
    void write_string( const std::string& str );
    template <size_t len>
    void write_array( const std::array<uint8_t, len>& );
    void write_blob( type_id_t, const void* buf, size_t len );

    template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    bool read_item( T& value );
    bool read_string( std::string& value );
    template <size_t len>
    bool read_array( std::array<uint8_t, len>& );
    bool read_blob( type_id_t, std::vector<uint8_t>& buf );

private:
    std::unique_ptr<common::SerializationBuffer> m_buff;
};

}  // namespace zwave
}  // namespace gateways
