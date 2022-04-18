
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

namespace common {
class IValue;
};

namespace gateways {

enum class ItemValueType {
    NoValue = 0,

    Int,
    Bool,
    Float,
    RGB,
    Usercode,
    String,
    CameraStream,
    Token,
    Illuminance,
    Pressure,
    Substance_amount,
    Power,
    Humidity,
    Velocity,
    Acceleration,
    Direction,
    General_purpose,
    Acidity,
    Electric_potential,
    Electric_current,
    Force,
    Irradiance,
    Precipitation,
    Length,
    Mass,
    Volume_flow,
    Volume,
    Angle,
    Frequency,
    Seismic_intensity,
    Seismic_magnitude,
    Ultraviolet,
    Electrical_resistance,
    Electrical_conductivity,
    Loudness,
    Moisture,
    Time,
    Radon_concentration,
    Blood_pressure,
    Energy,
    Rf_signal_strength,
    Temperature,
    Dictionary,
    Button_state,
    User_lock_operation,
    Array,
    Zwave_configuration,
    Amount_of_useful_energy,
    Kilo_volt_ampere_hour,
    Reactive_power_instant,
    Reactive_power_consumption,
    Action,

    LastValue
};

inline bool isScalable( const ItemValueType valueType ) {
    return ( ItemValueType::Illuminance <= valueType && ItemValueType::Temperature >= valueType ) ||
           ( ItemValueType::Amount_of_useful_energy <= valueType &&
             ItemValueType::Reactive_power_consumption >= valueType );
}

class BaseInfo {
    friend class ObjectSerializer;

public:
    using StringType = std::string;
    using MinMaxType = std::pair<float, float>;

    BaseInfo() = default;

    BaseInfo( StringType&& id, ItemValueType valueType ) {}

    BaseInfo( StringType&& deviceId, StringType&& id, ItemValueType valueType )
    : m_deviceId( deviceId )
    , m_id( id )
    , m_valueType( valueType ){};

    const StringType& getId() const {
        return m_id;
    }

    void setId( const char* id ) {
        m_id.assign( id );
    }

    void setId( const StringType& id ) {
        m_id.assign( id );
    }

    void setValueType( ItemValueType type ) {
        m_valueType = type;
    }

    ItemValueType getValueType() const {
        return m_valueType;
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
        m_deviceId.clear();
        m_valueType = ItemValueType::NoValue;
        setMinMax( { 0.0, 0.0 } );
        m_elementsMaxNumber = 0;
    }

    common::IValue::UPtr createDefaultValue() const;

    std::tuple<common::IValue::UPtr, int, std::string> createValue( const float percent ) const;

    common::IValue::UPtr castToType( common::IValue::UPtr&& ival, ItemValueType type ) const;
    common::IValue::UPtr castToItemType( common::IValue::UPtr&& ival ) const;

    virtual common::IValue::UPtr getValue() const;
    virtual common::IValue::UPtr getValueCache() const;
    virtual void setValue( std::unique_ptr<common::IValue> );

protected:
    StringType m_deviceId;

public:
    StringType m_id;
    ItemValueType m_valueType = ItemValueType::NoValue;
    MinMaxType m_minmax = { 0.0, 0.0 };
    int m_elementsMaxNumber = 0;
};

ItemValueType toItemValueType( const char* typeName );

inline ItemValueType toItemValueType( const std::string& typeName ) {
    return toItemValueType( typeName.c_str() );
}

inline bool isScalable( const std::string& typeName ) {
    return isScalable( toItemValueType( typeName.c_str() ) );
}

const char* toString( ItemValueType type );

}  // namespace gateways
