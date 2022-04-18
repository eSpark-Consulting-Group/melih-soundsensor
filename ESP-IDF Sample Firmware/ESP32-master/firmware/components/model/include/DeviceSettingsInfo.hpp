
#pragma once

#include <SettingsInfo.h>

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

#include <SettingsInfo.h>
#include "SettingStorage.h"

namespace common {
class IValue;
};

namespace gateways {

class DeviceSettingsInfo : public SettingsInfo {
    friend class ObjectSerializer;

public:
    using UPtr = std::unique_ptr<DeviceSettingsInfo>;
    DeviceSettingsInfo() = default;

    DeviceSettingsInfo( StringType settingId,
                        StringType deviceId,
                        LabelType label,
                        ItemValueType valueType,
                        std::optional<ItemValueType> elementType )
    : SettingsInfo( std::move( settingId ), std::move( deviceId ), label, valueType, elementType )
    , m_zwaveParameterNumber( 0 )
    , m_zwaveParameterSize( 0 ){};

    DeviceSettingsInfo( const DeviceSettingsInfo& other )
    : SettingsInfo( StringType( other.getItemId() ), StringType( other.getId() ), other.getValueType() )
    , m_zwaveParameterNumber( other.m_zwaveParameterNumber )
    , m_zwaveParameterSize( other.m_zwaveParameterSize ) {
        m_label = other.m_label;
        m_description = other.m_description;
        m_elementType = other.m_elementType;
        m_enums = other.m_enums;
        m_descriptor = other.getDescriptor();
        m_relative = other.m_relative;
        setMinMax( other.getMinMax() );
    }

    bool hasZwaveParameters() const {
        return m_zwaveParameterSize != 0;
    }

    int getZwaveParameterNumber() const {
        return m_zwaveParameterNumber;
    }

    void setZwaveParameterNumber( int newValue ) {
        m_zwaveParameterNumber = newValue;
    }

    int getZwaveParameterSize() const {
        return m_zwaveParameterSize;
    }

    void setZwaveParameterSize( int newValue ) {
        m_zwaveParameterSize = newValue;
    }

    common::IValue::UPtr getValue() const override {
        auto value = ::storage::DeviceSettings::get().getValue( getId() );
        return value ? std::move( value ) : createDefaultValue();
    }

    common::IValue::UPtr getValueCache() const override {
        return ::storage::DeviceSettings::get().getValue( getId() );
    }

    void setValue( std::unique_ptr<common::IValue> value ) override {
        ::storage::DeviceSettings::get().setValue( getId(), std::move( value ) );
    }

protected:
    int m_zwaveParameterNumber;  // Corresponding parameter number
    int m_zwaveParameterSize;    // Size of the corresponding parameter's value
};

}  // namespace gateways
