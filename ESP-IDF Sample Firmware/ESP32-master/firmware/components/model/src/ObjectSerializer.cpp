#include <ObjectSerializer.h>
#include <vector>
#include <DebugUtils.h>

#include <gason.hpp>
#include "CommonResources.h"
#include "IValueCast.h"
#include "values/IntValue.h"
#include "values/StringValue.h"
#include <memory>

namespace gateways {

constexpr char SERIALIZER_MAGIC = 0xBF;

using type_id_t = char;
using item_size_t = std::uint16_t;

enum class TYPE_ID : type_id_t { BOOL = 0u, INTEGER, FLOAT, STRING, TABLE, NIL };

template <typename T, typename std::enable_if<std::is_same<T, bool>::value, int>::type = 0>
constexpr TYPE_ID get_type_id( T ) {
    return TYPE_ID::BOOL;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, int>::type = 0>
constexpr TYPE_ID get_type_id( T ) {
    return TYPE_ID::INTEGER;
}

template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
constexpr TYPE_ID get_type_id( T ) {
    return TYPE_ID::FLOAT;
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type>
void ObjectSerializer::write_item( T item ) {
    m_buff->write( static_cast<type_id_t>( get_type_id( item ) ) );
    m_buff->write( item );
}

bool ObjectSerializer::read_item( bool& value ) {
    if( !m_buff->can_read( sizeof( type_id_t ) ) ) {
        return false;
    }
    TYPE_ID type = static_cast<TYPE_ID>( m_buff->read<type_id_t>() );
    if( type == TYPE_ID::BOOL && m_buff->can_read( sizeof( bool ) ) ) {
        value = m_buff->read<bool>();
        return true;
    }
    return false;
}

bool ObjectSerializer::read_item( float& value ) {
    if( !m_buff->can_read( sizeof( type_id_t ) ) ) {
        return false;
    }
    TYPE_ID type = static_cast<TYPE_ID>( m_buff->read<type_id_t>() );
    if( type == TYPE_ID::FLOAT && m_buff->can_read( sizeof( float ) ) ) {
        value = m_buff->read<float>();
        return true;
    }
    return false;
}

bool ObjectSerializer::read_item( int& value ) {
    if( !m_buff->can_read( sizeof( type_id_t ) ) ) {
        return false;
    }
    TYPE_ID type = static_cast<TYPE_ID>( m_buff->read<type_id_t>() );
    if( type == TYPE_ID::INTEGER && m_buff->can_read( sizeof( int ) ) ) {
        value = m_buff->read<int>();
        return true;
    }
    return false;
}

void ObjectSerializer::write_string( const void* str, std::size_t length ) {
    m_buff->write( static_cast<type_id_t>( TYPE_ID::STRING ) );
    m_buff->write<item_size_t>( length );
    m_buff->write( str, length );
}

void ObjectSerializer::write_string( const std::string& str ) {
    write_string( str.c_str(), str.length() );
}

void ObjectSerializer::write_ivalue( const ::common::IValue& value ) {
    auto valueStr = value.toJson(::common::resources::tokens::Value );
    write_string( valueStr.c_str(), valueStr.size() );
}

bool ObjectSerializer::read_string( std::string& value ) {
    if( !m_buff->can_read( sizeof( type_id_t ) ) ) {
        return false;
    }
    TYPE_ID type = static_cast<TYPE_ID>( m_buff->read<type_id_t>() );
    if( type == TYPE_ID::STRING && m_buff->can_read( sizeof( item_size_t ) ) ) {
        std::size_t length = m_buff->read<item_size_t>();
        const void* str;
        if( m_buff->can_read( length ) ) {
            m_buff->read( &str, length );

            value.clear();
            value.append( reinterpret_cast<const char*>( str ), length );
            return true;
        }
    }
    return false;
}

bool ObjectSerializer::read_ivalue( std::unique_ptr<::common::IValue>& value ) {
    std::string valueStr;
    if( read_string( valueStr ) ) {
        gason::JsonAllocator allocator;
        gason::JsonValue valueJson;
        if(::gason::JSON_PARSE_OK ==
           ::gason::jsonParse( const_cast<char*>( valueStr.c_str() ), valueJson, allocator ) ) {
            if( auto root = ::common::iValueCast<::common::TableValue>(::common::makeIval( valueJson ) ) ) {
                if( auto valueNode = ( *root )[::common::resources::tokens::Value] ) {
                    value = valueNode->clone();
                }
            }
        }
    }
    return value != nullptr;
}

void ObjectSerializer::write_table( const std::map<std::string, std::string>& table ) {
    m_buff->write( static_cast<type_id_t>( TYPE_ID::TABLE ) );
    m_buff->write<item_size_t>( table.size() );

    for( auto it = table.begin(); it != table.end(); it++ ) {
        std::string key = it->first;
        std::string value = it->second;
        write_string( key.c_str(), key.length() );
        write_string( value.c_str(), value.length() );
    }
}

bool ObjectSerializer::read_table( std::map<std::string, std::string>& table ) {
    if( !m_buff->can_read( sizeof( type_id_t ) ) ) {
        return false;
    }

    TYPE_ID type = static_cast<TYPE_ID>( m_buff->read<type_id_t>() );
    if( type == TYPE_ID::TABLE && m_buff->can_read( sizeof( item_size_t ) ) ) {
        std::size_t table_length = m_buff->read<item_size_t>();
        for( int i = 0; i < table_length; i++ ) {
            std::string key;
            if( read_string( key ) ) {
                std::string value;
                if( read_string( value ) ) {
                    table.insert( std::pair<std::string, std::string>( key, value ) );
                }
            }
        }
        return true;
    }

    return false;
}

std::vector<char> ObjectSerializer::serialize( ItemInfo& object ) {
    m_buff = std::make_unique<common::SerializationBuffer>();
    m_buff->write( SERIALIZER_MAGIC );

    write_string( object.getDeviceId() );
    write_string( object.getName() );
    write_item( static_cast<int>( object.getValueType() ) );
    write_item( object.hasGetter() );
    write_item( object.hasSetter() );
    // Added in EATT-1324
    write_item( object.isVisible() );
    // Added in EATT-1341
    auto enumsCount = object.getEnumItemsCount();
    write_item( enumsCount );
    for( int i = 0; i < enumsCount; ++i ) {
        const auto& enumVal = object.getEnumValue( i );
        if( enumVal.empty() ) {
            return {};
        }
        write_string( enumVal );
    }
    // Added in EATT-1481
    auto minmax = object.getMinMax();
    write_item( minmax.first );
    write_item( minmax.second );

    write_string( object.getUserCodeRestriction() );
    write_item( object.getElementsMaxNumber() );

    write_type_specific_fields( object );
    write_string( ItemInfo::statusToString( object.m_defaultStatus ) );
    FUNC_LOGI( "Default status saved %s", ItemInfo::statusToString( object.m_defaultStatus ) );

    return m_buff->extract();
}

bool ObjectSerializer::deserialize( std::vector<char>&& buff, ItemInfo& object ) {
    bool result{ true };
    object.Clear();
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

    result = result && read_string( object.m_deviceId );
    result = result && read_string( object.m_name );
    int value = 0;
    result = result && read_item( value );
    object.m_valueType = static_cast<ItemValueType>( value );
    result = result && read_item( object.m_hasGetter );
    result = result && read_item( object.m_hasSetter );
    if( !result ) {
        return false;
    }
    bool visible = true;
    // Added in EATT-1324
    result = result && read_item( visible );
    if( !result ) {
        return true;
    }
    object.setVisible( visible );
    // Added in EATT-1341
    int enumsCount = 0;
    result = result && read_item( enumsCount );
    if( !result ) {
        return true;
    }
    if( result && 0 < enumsCount ) {
        ItemInfo::EnumsArrayType enumValues;
        enumValues.reserve( enumsCount );
        for( int i = 0; i < enumsCount && result; ++i ) {
            std::string enumVal;
            result = result && read_string( enumVal );
            if( enumVal.empty() ) {
                return false;
            }
            enumValues.emplace_back( std::move( enumVal ) );
        }
        object.setEnumValues( std::move( enumValues ) );
    }
    // Added in EATT-1481
    float minVal = 0.0, maxVal = 0.0;
    result = result && read_item( minVal );
    result = result && read_item( maxVal );
    if( !result ) {
        return true;
    }
    object.setMinMax( { minVal, maxVal } );

    result = result && read_string( object.m_userCodeRestriction );
    result = result && read_item( object.m_elementsMaxNumber );
    if( !result ) {
        return true;
    }

    read_type_specific_fields( object );

    std::string status;
    if( !read_string( status ) ) {
        object.m_defaultStatus = ItemInfo::Status::outdated;
    }
    else {
        object.m_defaultStatus = ItemInfo::stringToStatus( status.c_str() ) == ItemInfo::Status::unknown ?
                                     ItemInfo::Status::outdated :
                                     ItemInfo::stringToStatus( status.c_str() );
    }

    return result;
}

std::vector<char> ObjectSerializer::serialize( BaseDeviceInfo& object ) {
    m_buff = std::make_unique<common::SerializationBuffer>();
    m_buff->write( SERIALIZER_MAGIC );

    write_string( object.m_gatewayId.c_str(), object.m_gatewayId.length() );
    write_string( object.m_parentDeviceId.c_str(), object.m_parentDeviceId.length() );
    write_string( object.m_deviceTypeId.c_str(), object.m_deviceTypeId.length() );
    write_string( object.m_name.c_str(), object.m_name.length() );
    write_string( object.m_category.c_str(), object.m_category.length() );
    write_string( object.m_subCategory.c_str(), object.m_subCategory.length() );
    write_string( object.m_type.c_str(), object.m_type.length() );
    write_string( object.m_roomId.c_str(), object.m_roomId.length() );
    write_item( object.isBatteryPowered() );                                // Added in EATT-803
    write_item( object.isPersistent() );                                    // Added in EATT-1337
    write_item( object.isServiceNotification() );                           // Added in EATT-1539
    write_string( object.m_security.c_str(), object.m_security.length() );  // Added in EATT-1554
    write_string( object.m_status.c_str(), object.m_status.length() );      // Added in EATT-2121
    write_table( object.m_info );
    write_table( object.m_firmware );

    return m_buff->extract();
}

bool ObjectSerializer::deserialize( const std::string& id, std::vector<char>&& buff, BaseDeviceInfo& object ) {
    object.m_deviceId = id;

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

    if( !read_string( object.m_gatewayId ) || !read_string( object.m_parentDeviceId ) ||
        !read_string( object.m_deviceTypeId ) || !read_string( object.m_name ) || !read_string( object.m_category ) ||
        !read_string( object.m_subCategory ) || !read_string( object.m_type ) || !read_string( object.m_roomId ) ) {
        FUNC_LOGE( "Not enought data in buffer to deserialize device info!" );
        return false;
    }

    // Added in EATT-803
    bool isSuccessfully{ true };
    {
        bool isBatteryPowered;
        isSuccessfully = read_item( isBatteryPowered );
        object.setBatteryPowered( isSuccessfully ? isBatteryPowered : false );
    }

    // Added in EATT-1337
    if( isSuccessfully ) {
        bool isPersistent;
        isSuccessfully = read_item( isPersistent );
        object.setPersistent( isSuccessfully ? isPersistent : false );
    }

    // Added in EATT-1539
    if( isSuccessfully ) {
        bool isServiceNotification;
        isSuccessfully = read_item( isServiceNotification );
        object.setServiceNotification( isSuccessfully ? isServiceNotification : false );
    }

    // Added in EATT-1554
    if( isSuccessfully ) {
        std::string security;
        isSuccessfully = read_string( security );
        object.setSecurity( isSuccessfully ? security.c_str() : "no" );
    }

    // Added in EATT-2121
    if( isSuccessfully ) {
        std::string status;
        isSuccessfully = read_string( status );
        object.setStatus( isSuccessfully ? status.c_str() : "idle" );
    }

    if( isSuccessfully ) {
        std::map<std::string, std::string> info;
        read_table( info );
        object.setInfo( info );
    }

    if( isSuccessfully ) {
        std::map<std::string, std::string> firmware;
        read_table( firmware );
        object.setFirmware( firmware );
    }

    return true;
}

std::vector<char> ObjectSerializer::serialize( const DeviceSettingsInfo& object ) {
    m_buff = std::make_unique<common::SerializationBuffer>();
    m_buff->write( SERIALIZER_MAGIC );

    write_string( object.getItemId() );
    gateways::SettingsInfo::LabelType label = object.getLabel();
    write_string( label.lang_tag );
    write_string( label.text );
    write_item( static_cast<int>( object.getValueType() ) );

    auto enumsCount = object.getEnumItemsCount();
    write_item( enumsCount );
    for( int i = 0; i < enumsCount; ++i ) {
        const auto& enumVal = object.getEnumValue( i );
        write_string( enumVal.first );
        write_string( enumVal.second.lang_tag );
        write_string( enumVal.second.text );
    }
    auto minmax = object.getMinMax();
    write_item( minmax.first );
    write_item( minmax.second );

    write_item( object.getElementsMaxNumber() );

    gateways::SettingsInfo::LabelType description = object.getDescription();
    write_string( description.lang_tag );
    write_string( description.text );

    write_item( static_cast<int>( object.getElementType() ) );

    write_item( object.m_zwaveParameterNumber );
    write_item( object.m_zwaveParameterSize );

    write_item( object.hasDescriptor() );
    if( object.hasDescriptor() ) {
        write_ivalue( *object.getDescriptor() );
    }
    int relative = object.m_relative;
    write_item( relative );
    return m_buff->extract();
}

bool ObjectSerializer::deserialize( std::vector<char>&& buff, DeviceSettingsInfo& object ) {
    bool result{ true };

    object.Clear();
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

    result = result && read_string( object.m_itemId );
    result = result && read_string( object.m_label.lang_tag );
    result = result && read_string( object.m_label.text );
    int value = 0;
    result = result && read_item( value );
    object.m_valueType = static_cast<ItemValueType>( value );
    if( !result ) {
        return false;
    }
    int enumsCount = 0;
    result = result && read_item( enumsCount );
    if( !result ) {
        return true;
    }
    if( result && 0 < enumsCount ) {
        SettingsInfo::EnumsArrayType enumValues;
        enumValues.reserve( enumsCount );
        for( int i = 0; i < enumsCount && result; ++i ) {
            SettingsInfo::EnumType enumVal;
            result = result && read_string( enumVal.first );
            if( enumVal.first.empty() ) {
                return false;
            }
            result = result && read_string( enumVal.second.lang_tag );
            result = result && read_string( enumVal.second.text );
            enumValues.emplace_back( std::move( enumVal ) );
        }
        object.setEnumValues( std::move( enumValues ) );
    }
    float minVal = 0.0, maxVal = 0.0;
    result = result && read_item( minVal );
    result = result && read_item( maxVal );
    if( !result ) {
        return true;
    }
    object.setMinMax( { minVal, maxVal } );

    result = result && read_item( object.m_elementsMaxNumber );
    if( !result ) {
        return true;
    }

    result = result && read_string( object.m_description.lang_tag );
    result = result && read_string( object.m_description.text );

    int element_type = 0;
    result = result && read_item( element_type );
    object.m_elementType = static_cast<ItemValueType>( element_type );

    if( !result ) {
        return true;
    }

    result = read_item( value );
    object.setZwaveParameterNumber( result ? value : 0 );

    if( result ) {
        result = read_item( value );
        object.setZwaveParameterSize( result ? value : 0 );
    }

    bool hasDescriptor = false;
    result = result && read_item( hasDescriptor );
    if( result && hasDescriptor ) {
        result = result && read_ivalue( object.m_descriptor );
    }
    int relative;
    result = read_item( relative );
    object.m_relative = relative > 0 ? true : false;
    return true;
}

bool ObjectSerializer::deserialize( std::vector<char>&& buff, GatewaySettingsInfo& object ) {
    bool result{ true };

    object.Clear();
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

    result = result && read_string( object.m_itemId );
    result = result && read_string( object.m_label.lang_tag );
    result = result && read_string( object.m_label.text );
    int value = 0;
    result = result && read_item( value );
    object.m_valueType = static_cast<ItemValueType>( value );
    if( !result ) {
        return false;
    }
    int enumsCount = 0;
    result = result && read_item( enumsCount );
    if( !result ) {
        return true;
    }
    if( result && 0 < enumsCount ) {
        SettingsInfo::EnumsArrayType enumValues;
        enumValues.reserve( enumsCount );
        for( int i = 0; i < enumsCount && result; ++i ) {
            SettingsInfo::EnumType enumVal;
            result = result && read_string( enumVal.first );
            if( enumVal.first.empty() ) {
                return false;
            }
            result = result && read_string( enumVal.second.lang_tag );
            result = result && read_string( enumVal.second.text );
            enumValues.emplace_back( std::move( enumVal ) );
        }
        object.setEnumValues( std::move( enumValues ) );
    }
    float minVal = 0.0, maxVal = 0.0;
    result = result && read_item( minVal );
    result = result && read_item( maxVal );
    if( !result ) {
        return true;
    }

    object.setMinMax( { minVal, maxVal } );

    result = result && read_item( object.m_elementsMaxNumber );
    if( !result ) {
        return true;
    }

    result = result && read_string( object.m_description.lang_tag );
    result = result && read_string( object.m_description.text );

    int element_type = 0;
    result = result && read_item( element_type );
    object.m_elementType = static_cast<ItemValueType>( element_type );

    if( !result ) {
        return true;
    }

    return true;
}

std::vector<char> ObjectSerializer::serialize( const GatewaySettingsInfo& object ) {
    m_buff = std::make_unique<common::SerializationBuffer>();
    m_buff->write( SERIALIZER_MAGIC );

    write_string( object.getItemId() );
    SettingsInfo::LabelType label = object.getLabel();
    write_string( label.lang_tag );
    write_string( label.text );
    write_item( static_cast<int>( object.getValueType() ) );

    auto enumsCount = object.getEnumItemsCount();
    write_item( enumsCount );
    for( int i = 0; i < enumsCount; ++i ) {
        const auto& enumVal = object.getEnumValue( i );
        write_string( enumVal.first );
        write_string( enumVal.second.lang_tag );
        write_string( enumVal.second.text );
    }
    auto minmax = object.getMinMax();
    write_item( minmax.first );
    write_item( minmax.second );

    write_item( object.getElementsMaxNumber() );

    SettingsInfo::LabelType description = object.getDescription();
    write_string( description.lang_tag );
    write_string( description.text );

    write_item( static_cast<int>( object.getElementType() ) );

    return m_buff->extract();
}

void ObjectSerializer::write_type_specific_fields( const ItemInfo& itemInfo ) {
    namespace tokens = ::common::resources::tokens;
    auto& fields = itemInfo.getTypeSpecificFields();
    if( tokens::WeeklyUserCodeIntervals == itemInfo.getName() ||
        tokens::DailyUserCodeIntervals == itemInfo.getName() ) {
        auto iterElementsMaxNumberPerArray = fields.find( tokens::ElementsMaxNumberPerArray );
        if( iterElementsMaxNumberPerArray != fields.end() ) {
            if( auto elementsMaxNumberPerArray =
                    ::common::iValueCast<::common::IntValue>( iterElementsMaxNumberPerArray->second.get() ) ) {
                write_item( elementsMaxNumberPerArray->get() );
            }
            else {
                FUNC_LOGW( "%s cannot cast to int", tokens::ElementsMaxNumberPerArray );
            }
        }
        else {
            FUNC_LOGW( "%s is missing", tokens::ElementsMaxNumberPerArray );
            return;
        }
        auto iterOneWeekDayCost = fields.find( tokens::OneWeekDayCost );
        auto iterOneShiftedWeekDayCost = fields.find( tokens::OneShiftedWeekDayCost );
        if( iterOneWeekDayCost != fields.end() && iterOneShiftedWeekDayCost != fields.end() ) {
            auto oneWeekDayCost = ::common::iValueCast<::common::IntValue>( iterOneWeekDayCost->second.get() );
            auto oneShiftedWeekDayCost =
                ::common::iValueCast<::common::IntValue>( iterOneShiftedWeekDayCost->second.get() );
            if( oneWeekDayCost && oneShiftedWeekDayCost ) {
                write_item( oneWeekDayCost->get() );
                write_item( oneShiftedWeekDayCost->get() );
            }
        }
    }
    else {
        write_item( static_cast<int>( fields.size() ) );
        for( auto& field : fields ) {
            write_string( field.first );
            write_ivalue( *field.second );
        }
    }
}

void ObjectSerializer::read_type_specific_fields( ItemInfo& itemInfo ) {
    namespace tokens = ::common::resources::tokens;
    if( tokens::WeeklyUserCodeIntervals == itemInfo.getName() ||
        tokens::DailyUserCodeIntervals == itemInfo.getName() ) {
        int elementsMaxNumberPerArray, oneWeekDayCost, oneShiftedWeekDayCost;
        if( read_item( elementsMaxNumberPerArray ) ) {
            itemInfo.setTypeSpecificField( tokens::ElementsMaxNumberPerArray,
                                           std::make_unique<::common::IntValue>( elementsMaxNumberPerArray ) );
        }
        else {
            FUNC_LOGW( "%s is missing", tokens::ElementsMaxNumberPerArray );
            return;
        }
        if( read_item( oneWeekDayCost ) && read_item( oneShiftedWeekDayCost ) ) {
            itemInfo.setTypeSpecificField( tokens::OneWeekDayCost,
                                           std::make_unique<::common::IntValue>( oneWeekDayCost ) );
            itemInfo.setTypeSpecificField( tokens::OneShiftedWeekDayCost,
                                           std::make_unique<::common::IntValue>( oneShiftedWeekDayCost ) );
        }
    }
    else {
        int numberOfFields = 0;
        if( read_item( numberOfFields ) ) {
            for( size_t i = 0; i < numberOfFields; ++i ) {
                std::string key;
                std::unique_ptr<::common::IValue> value;
                if( read_string( key ) && read_ivalue( value ) ) {
                    itemInfo.setTypeSpecificField( key, std::move( value ) );
                }
            }
        }
    }
}

}  // namespace gateways
