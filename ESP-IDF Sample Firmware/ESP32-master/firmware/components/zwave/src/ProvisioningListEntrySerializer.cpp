#include "ProvisioningListEntrySerializer.h"

#include <bitset>
#include <znet/znet_provisioning_list_entry.hpp>

#include "DebugUtils.h"

constexpr char SERIALIZER_MAGIC = 0xAF;

namespace gateways {
namespace zwave {

enum class FieldBits {
    NODE,
    STATUS,
    ADDING_MODE,
    PRODUCT,
    TYPE,
    MAX_REQUEST_INTERVAL,
    UUID,
    NAME,
    ROOM,
    SECURITY_MODES,
    NODE_SECURITY_MODES,
    SUPPORTED_SECURITY_MODES,
    UUID_INFORMATION,
    INCLUSION_SETTING,
    LOCATION,
    SUPPORTED_ADDING_MODES,
    MAX_BIT
};

bool ProvisioningListEntrySerializer::deserialize( ProvisioningListEntrySerializer::SerializedParams&& buff,
                                                   znet::ProvisioningListEntry& entry ) {
    m_buff = std::make_unique<common::SerializationBuffer>();
    m_buff->init( std::move( buff ) );

    if( !m_buff->can_read( sizeof( char ) ) ) {
        m_buff.reset();
        FUNC_LOGE( "Reading has been  FAILED" );
        return false;
    }

    if( m_buff->read<char>() != SERIALIZER_MAGIC ) {
        m_buff.reset();
        FUNC_LOGE( "Checking magic number has been FAILED" );
        return false;
    }

    bool result{ true };
    int value = 0;

    result = result && read_item( entry.m_provisioningId );
    if( !result ) {
        return false;
    }

    for( auto& dskItem : entry.m_dsk ) {
        result = result && read_item( value );
        if( !result ) {
            return false;
        }
        dskItem = static_cast<uint16_t>( value );
    }

    unsigned long bitsetData = 0;
    result = result && read_item( bitsetData );
    if( !result ) {
        return false;
    }
    std::bitset<static_cast<size_t>( FieldBits::MAX_BIT )> bitset{ bitsetData };
    if( bitset[static_cast<size_t>( FieldBits::NODE )] ) {
        entry.m_fields.m_node = 0;
        result = result && read_item( *entry.m_fields.m_node );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::STATUS )] ) {
        result = result && read_item( value );
        if( !result ) {
            return false;
        }
        entry.m_fields.m_status = static_cast<::znet::ProvisioningListEntryFields::Status>( value );
    }
    if( bitset[static_cast<size_t>( FieldBits::ADDING_MODE )] ) {
        result = result && read_item( value );
        if( !result ) {
            return false;
        }
        entry.m_fields.m_addingMode = static_cast<::znet::ProvisioningListEntryFields::AddingMode>( value );
    }
    if( bitset[static_cast<size_t>( FieldBits::PRODUCT )] ) {
        entry.m_fields.m_product = znet::ProvisioningListEntryFields::Product{ 0, 0, 0, 0, 0 };
        result = result && read_item( entry.m_fields.m_product->m_manufacturerId );
        result = result && read_item( entry.m_fields.m_product->m_productType );
        result = result && read_item( entry.m_fields.m_product->m_productId );
        result = result && read_item( entry.m_fields.m_product->m_version );
        result = result && read_item( entry.m_fields.m_product->m_subversion );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::TYPE )] ) {
        entry.m_fields.m_type = znet::ProvisioningListEntryFields::Type{ 0, 0, 0 };
        result = result && read_item( entry.m_fields.m_type->m_generic );
        result = result && read_item( entry.m_fields.m_type->m_specific );
        result = result && read_item( entry.m_fields.m_type->m_icon );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::MAX_REQUEST_INTERVAL )] ) {
        entry.m_fields.m_maxRequestInterval = 0;
        result = result && read_item( *entry.m_fields.m_maxRequestInterval );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::UUID )] ) {
        entry.m_fields.m_uuid = std::string{};
        result = result && read_string( *entry.m_fields.m_uuid );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::NAME )] ) {
        entry.m_fields.m_name = std::string{};
        result = result && read_string( *entry.m_fields.m_name );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::ROOM )] ) {
        entry.m_fields.m_room = std::string{};
        result = result && read_string( *entry.m_fields.m_room );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::SECURITY_MODES )] ) {
        entry.m_fields.m_securityModes = 0;
        result = result && read_item( *entry.m_fields.m_securityModes );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::NODE_SECURITY_MODES )] ) {
        entry.m_fields.m_nodeSecurityModes = 0;
        result = result && read_item( *entry.m_fields.m_nodeSecurityModes );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::SUPPORTED_SECURITY_MODES )] ) {
        entry.m_fields.m_supportedModes = 0;
        result = result && read_item( *entry.m_fields.m_supportedModes );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::UUID_INFORMATION )] ) {
        entry.m_fields.m_uuidInformation = ::znet::ProvisioningListEntryFields::UuidInformation{};
        result = result && read_item( entry.m_fields.m_uuidInformation->m_presentationFormat );
        result = result && read_array( entry.m_fields.m_uuidInformation->m_uuid );
        if( !result ) {
            return false;
        }
    }
    if( bitset[static_cast<size_t>( FieldBits::INCLUSION_SETTING )] ) {
        result = result && read_item( value );
        if( !result ) {
            return false;
        }
        entry.m_fields.m_inclusionSetting = static_cast<::znet::ProvisioningListEntryFields::InclusionSetting>( value );
    }
    if( bitset[static_cast<size_t>( FieldBits::LOCATION )] ) {
        entry.m_fields.m_location = std::string{};
        result = result && read_string( *entry.m_fields.m_location );
        if( !result ) {
            return false;
        }
    }

    if( bitset[static_cast<size_t>( FieldBits::SUPPORTED_ADDING_MODES )] ) {
        std::vector<uint8_t> buf;
        result = result && read_blob( static_cast<type_id_t>( TYPE_ID::BLOB ), buf );
        if( !result ) {
            return false;
        }

        entry.m_fields.m_supportedAddingModes.clear();
        for( auto& it : buf ) {
            entry.m_fields.m_supportedAddingModes.push_back(
                static_cast<::znet::ProvisioningListEntryFields::AddingMode>( it ) );
        }
    }

    return result;
}

ProvisioningListEntrySerializer::SerializedParams ProvisioningListEntrySerializer::serialize(
    const znet::ProvisioningListEntry& entry ) {
    m_buff = std::make_unique<common::SerializationBuffer>();
    m_buff->write( SERIALIZER_MAGIC );

    write_item( entry.m_provisioningId );

    for( auto dskItem : entry.m_dsk ) {
        write_item( static_cast<int>( dskItem ) );
    }

    std::bitset<static_cast<size_t>( FieldBits::MAX_BIT )> bitset;

    bitset[static_cast<size_t>( FieldBits::NODE )] = entry.m_fields.m_node.has_value();
    bitset[static_cast<size_t>( FieldBits::STATUS )] = entry.m_fields.m_status.has_value();
    bitset[static_cast<size_t>( FieldBits::ADDING_MODE )] = entry.m_fields.m_addingMode.has_value();
    bitset[static_cast<size_t>( FieldBits::PRODUCT )] = entry.m_fields.m_product.has_value();
    bitset[static_cast<size_t>( FieldBits::TYPE )] = entry.m_fields.m_type.has_value();
    bitset[static_cast<size_t>( FieldBits::MAX_REQUEST_INTERVAL )] = entry.m_fields.m_maxRequestInterval.has_value();
    bitset[static_cast<size_t>( FieldBits::UUID )] = entry.m_fields.m_uuid.has_value();
    bitset[static_cast<size_t>( FieldBits::NAME )] = entry.m_fields.m_name.has_value();
    bitset[static_cast<size_t>( FieldBits::ROOM )] = entry.m_fields.m_room.has_value();
    bitset[static_cast<size_t>( FieldBits::SECURITY_MODES )] = entry.m_fields.m_securityModes.has_value();
    bitset[static_cast<size_t>( FieldBits::NODE_SECURITY_MODES )] = entry.m_fields.m_nodeSecurityModes.has_value();
    bitset[static_cast<size_t>( FieldBits::SUPPORTED_SECURITY_MODES )] = entry.m_fields.m_supportedModes.has_value();
    bitset[static_cast<size_t>( FieldBits::UUID_INFORMATION )] = entry.m_fields.m_uuidInformation.has_value();
    bitset[static_cast<size_t>( FieldBits::INCLUSION_SETTING )] = entry.m_fields.m_inclusionSetting.has_value();
    bitset[static_cast<size_t>( FieldBits::LOCATION )] = entry.m_fields.m_location.has_value();
    bitset[static_cast<size_t>( FieldBits::SUPPORTED_ADDING_MODES )] = entry.m_fields.m_supportedAddingModes.size() > 0;

    write_item( bitset.to_ulong() );

    if( entry.m_fields.m_node ) {
        write_item( *entry.m_fields.m_node );
    }

    if( entry.m_fields.m_status ) {
        write_item( static_cast<int>( *entry.m_fields.m_status ) );
    }

    if( entry.m_fields.m_addingMode ) {
        write_item( static_cast<int>( *entry.m_fields.m_addingMode ) );
    }

    if( entry.m_fields.m_product ) {
        write_item( entry.m_fields.m_product->m_manufacturerId );
        write_item( entry.m_fields.m_product->m_productType );
        write_item( entry.m_fields.m_product->m_productId );
        write_item( entry.m_fields.m_product->m_version );
        write_item( entry.m_fields.m_product->m_subversion );
    }

    if( entry.m_fields.m_type ) {
        write_item( entry.m_fields.m_type->m_generic );
        write_item( entry.m_fields.m_type->m_specific );
        write_item( entry.m_fields.m_type->m_icon );
    }

    if( entry.m_fields.m_maxRequestInterval ) {
        write_item( *entry.m_fields.m_maxRequestInterval );
    }

    if( entry.m_fields.m_uuid ) {
        write_string( *entry.m_fields.m_uuid );
    }

    if( entry.m_fields.m_name ) {
        write_string( *entry.m_fields.m_name );
    }

    if( entry.m_fields.m_room ) {
        write_string( *entry.m_fields.m_room );
    }

    if( entry.m_fields.m_securityModes ) {
        write_item( *entry.m_fields.m_securityModes );
    }

    if( entry.m_fields.m_nodeSecurityModes ) {
        write_item( *entry.m_fields.m_nodeSecurityModes );
    }

    if( entry.m_fields.m_supportedModes ) {
        write_item( *entry.m_fields.m_supportedModes );
    }

    if( entry.m_fields.m_uuidInformation ) {
        write_item( entry.m_fields.m_uuidInformation->m_presentationFormat );
        write_array( entry.m_fields.m_uuidInformation->m_uuid );
    }

    if( entry.m_fields.m_inclusionSetting ) {
        write_item( static_cast<int>( *entry.m_fields.m_inclusionSetting ) );
    }

    if( entry.m_fields.m_location ) {
        write_string( *entry.m_fields.m_location );
    }

    if( entry.m_fields.m_supportedAddingModes.size() > 0 ) {
        std::vector<uint8_t> buf;
        for( auto& it : entry.m_fields.m_supportedAddingModes ) {
            buf.push_back( static_cast<uint8_t>( it ) );
        }
        write_blob( static_cast<type_id_t>( TYPE_ID::BLOB ), buf.data(), buf.size() );
    }
    return m_buff->extract();
}

template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type>
void ProvisioningListEntrySerializer::write_item( T item ) {
    m_buff->write( static_cast<type_id_t>( TYPE_ID::INTEGER ) );
    m_buff->write( item );
}

void ProvisioningListEntrySerializer::write_blob( type_id_t type, const void* buf, size_t len ) {
    m_buff->write( type );
    m_buff->write<item_size_t>( len );
    m_buff->write( buf, len );
}

void ProvisioningListEntrySerializer::write_string( const std::string& str ) {
    write_blob( static_cast<type_id_t>( TYPE_ID::STRING ), str.c_str(), str.length() );
}

template <size_t len>
void ProvisioningListEntrySerializer::write_array( const std::array<uint8_t, len>& data ) {
    write_blob( static_cast<type_id_t>( TYPE_ID::BLOB ), data.data(), data.size() );
}

template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type>
bool ProvisioningListEntrySerializer::read_item( T& value ) {
    if( !m_buff->can_read( sizeof( type_id_t ) ) ) {
        FUNC_LOGE( "read_item OUT" );
        return false;
    }
    TYPE_ID type = static_cast<TYPE_ID>( m_buff->read<type_id_t>() );
    if( type == TYPE_ID::INTEGER && m_buff->can_read( sizeof( T ) ) ) {
        value = m_buff->read<T>();
        return true;
    }
    FUNC_LOGE( "read  FAILED" );
    return false;
}

bool ProvisioningListEntrySerializer::read_string( std::string& value ) {
    std::vector<uint8_t> buf;
    if( read_blob( static_cast<type_id_t>( TYPE_ID::STRING ), buf ) ) {
        buf.push_back( 0 );
        value = reinterpret_cast<const char*>( buf.data() );
        return true;
    }
    return false;
}

template <size_t len>
bool ProvisioningListEntrySerializer::read_array( std::array<uint8_t, len>& outBuf ) {
    std::vector<uint8_t> buf;
    if( read_blob( static_cast<type_id_t>( TYPE_ID::BLOB ), buf ) && outBuf.size() == buf.size() ) {
        std::copy( buf.begin(), buf.end(), outBuf.begin() );
        return true;
    }
    return false;
}

bool ProvisioningListEntrySerializer::read_blob( type_id_t expectedType, std::vector<uint8_t>& buf ) {
    if( !m_buff->can_read( sizeof( type_id_t ) ) ) {
        return false;
    }
    auto actualType = m_buff->read<type_id_t>();
    if( expectedType == actualType && m_buff->can_read( sizeof( item_size_t ) ) ) {
        std::size_t dataLen = m_buff->read<item_size_t>();
        const void* data;
        if( m_buff->can_read( dataLen ) ) {
            m_buff->read( &data, dataLen );

            buf.resize( dataLen, 0 );
            auto byteData = static_cast<const uint8_t*>( data );
            std::copy( byteData, byteData + dataLen, buf.data() );
            return true;
        }
    }
    return false;
}

}  // namespace zwave
}  // namespace gateways
