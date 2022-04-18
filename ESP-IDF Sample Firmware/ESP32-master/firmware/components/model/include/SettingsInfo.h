
#pragma once

#include <GatewayDefs.h>
#include <GatewayID.h>
#include <Utils.h>
#include <SerializationBuffer.h>

#include "IValue.h"
#include "IValueCast.h"
#include "values/UserCodeValue.h"

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <functional>
#include <unordered_map>
#include <MemCache.h>
#include "CommonResources.h"
#include <DebugUtils.h>
#include <BaseInfo.h>

namespace common {
class IValue;
};

namespace gateways {

class SettingsInfo : public BaseInfo {
    friend class ObjectSerializer;

public:
    using StringType = std::string;
    using MinMaxType = std::pair<float, float>;

    typedef struct {
        StringType lang_tag;
        StringType text;
    } LabelType;

    using EnumType = std::pair<StringType, LabelType>;
    using EnumsArrayType = std::vector<EnumType>;
    using UPtr = std::unique_ptr<SettingsInfo>;
    using ValueT = common::IValue::UPtr;

    SettingsInfo() = default;

    SettingsInfo( StringType settingId,
                  StringType itemId,
                  LabelType label,
                  ItemValueType valueType,
                  std::optional<ItemValueType> elementType )

    : BaseInfo( "", std::move( settingId ), valueType )
    , m_itemId( std::move( itemId ) )
    , m_label( label )
    , m_elementType( elementType )
    , m_relative( false ) {
        m_enums.reserve( 1 );
    }

    SettingsInfo( StringType itemId, StringType Id, ItemValueType valueType )
    : BaseInfo( std::move( itemId ), std::move( Id ), valueType )
    , m_relative( false ) {
        m_itemId = m_deviceId;
    }

    SettingsInfo( const SettingsInfo& other )
    : BaseInfo( StringType( other.getItemId() ), StringType( other.getId() ), other.getValueType() ) {
        m_enums = other.m_enums;
        m_descriptor = other.getDescriptor();
        m_relative = other.isRelative();
    }

    const StringType& getItemId() const {
        return m_itemId;
    }

    void setItemId( const StringType& id ) {
        m_itemId.assign( id );
    }

    bool isScalable() const {
        return ::gateways::isScalable( m_valueType );
    }

    bool isMinMax() const {
        return m_minmax.first != m_minmax.second;
    }

    void setMinMax( const MinMaxType minmax ) {
        m_minmax = minmax;
    }

    MinMaxType getMinMax() const {
        return m_minmax;
    }

    void setElementsMaxNumber( uint8_t maxNumber ) {
        m_elementsMaxNumber = maxNumber;
    }

    int getElementsMaxNumber() const {
        return m_elementsMaxNumber;
    }

    void Clear() {
        BaseInfo::Clear();
        m_itemId.clear();
    }

    bool isEnum() const {
        return !m_enums.empty();
    }

    int getEnumItemsCount() const {
        return m_enums.size();
    }

    const EnumType& getEnumValue( uint8_t idx ) const {
        return m_enums[idx];
    }

    void setEnumValues( common::TableValue*& table_ptr );

    void setEnumValues( EnumsArrayType&& values ) {
        m_enums = std::move( values );
    }

    bool checkEnumValue( const StringType& value ) const;

    void setLabel( LabelType label ) {
        m_label = label;
    }

    LabelType getLabel() const {
        return m_label;
    }

    void setDescription( const StringType& lang_tag, const StringType& text ) {
        m_description.lang_tag = lang_tag;
        m_description.text = text;
    }

    LabelType getDescription() const {
        return m_description;
    }

    bool isDescription() const {
        return !( m_description.lang_tag.empty() && m_description.text.empty() );
    }

    bool hasDescriptor() const {
        return m_descriptor != nullptr;
    }

    std::unique_ptr<::common::IValue> getDescriptor() const {
        if( !m_descriptor ) {
            return std::make_unique<::common::BoolValue>( false );
        }
        return m_descriptor->clone();
    }

    void setDescriptor( std::unique_ptr<::common::IValue> newDescriptor ) {
        m_descriptor = std::move( newDescriptor );
    }

    void setElementType( ItemValueType type ) {
        m_elementType = type;
    }

    ItemValueType getElementType() const {
        return m_elementType ? *m_elementType : ItemValueType::NoValue;
    }

    common::IValue::UPtr castToElementType( common::IValue::UPtr&& ival ) const;

    bool isRelative() const {
        return m_relative;
    }

    void setRelative( bool relative ) {
        m_relative = relative;
    }

protected:
    StringType m_itemId;
    StringType m_settingId;   // Id of device which own of this setting
    LabelType m_label;        // Main title of setting
    LabelType m_description;  // Detailed description of setting
    std::optional<ItemValueType> m_elementType;
    EnumsArrayType m_enums;  // Possible values for token type
                             // MinMaxType m_minmax = { 0.0, 0.0 };  // Min Max value for some m_value
    std::unique_ptr<::common::IValue> m_descriptor;
    bool m_relative;
};

}  // namespace gateways
