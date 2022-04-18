
#include <DeviceInfo.h>
#include <IValueCast.h>
#include <HouseModesManager.h>
#include <CommonResources.h>
#include <Cache.h>
#include <StorageFile.h>
#include <Storage.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <DeviceInfo.h>
#include <ObjectSerializer.h>
#include "converters/ScalesConverterVisitor.hpp"
#include "HubSettings.h"

#include "values/Int64Value.hpp"

#include <cmath>

#include <DebugUtils.h>

namespace gateways {

using namespace common::resources;

namespace {

constexpr int32_t BAD_USER_ID = -1;

class DeltaValueSetter : public common::IValueVisitor {
    float m_percent;
    ItemInfo::MinMaxType m_minmax;
    int m_errorCode = common::resources::errors::NotError;
    std::string m_errorMessage;

    template <typename T>
    void setValue( T& value ) {
        auto oldValue = value.get();
        float delta = m_percent * ( m_minmax.second - m_minmax.first ) / 100.0;
        if( ( float( oldValue ) >= m_minmax.second && 0.0 <= delta ) ||
            ( float( oldValue ) <= m_minmax.first && 0.0 >= delta ) ) {
            m_errorCode = common::resources::errors::InvalidParams;
            m_errorMessage = common::resources::errors::data::RangeInvalid;
            return;
        }

        decltype( oldValue ) newValue = oldValue + delta;
        // Correction "delta" rounding issues
        if( newValue == oldValue ) {
            newValue += std::copysign( 1, m_percent );
        }

        if( float( newValue ) > m_minmax.second ) {
            newValue = m_minmax.second;
        }
        else if( float( newValue ) < m_minmax.first ) {
            newValue = m_minmax.first;
        }
        value.set( newValue );
    }

public:
    DeltaValueSetter( const float percent, ItemInfo::MinMaxType minmax )
    : m_percent( percent )
    , m_minmax( minmax ) {}

    void visit( common::FloatValue& value ) override {
        setValue( value );
    };

    void visit( common::ScalebleValue& value ) override {
        setValue( value );
    };

    void visit( common::IntValue& value ) override {
        setValue( value );
    };

    void visit( common::Int64Value& value ) override {
        setValue( value );
    };

    void visit( common::BoolValue& value ) override {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for BoolValue!";
    }

    void visit( common::StringValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for StringValue!";
    }

    void visit( common::TokenValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for TokenValue!";
    }

    void visit( common::RgbValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for RgbValue!";
    }

    void visit( common::NotificationValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for NotificationValue!";
    }

    void visit( common::UserCodeValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for UserCodeValue!";
    }

    void visit( common::ArrayValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for ArrayValue!";
    }

    void visit( common::TableValue& value ) {
        m_errorCode = common::resources::errors::InvalidParams;
        m_errorMessage = "DeltaValueSetter wasn't implemented for TableValue!";
    }

    int errorCode() const {
        return m_errorCode;
    }

    std::string& errorMessage() {
        return m_errorMessage;
    }
};

}  // namespace

common::IValue::UPtr BaseInfo::getValue() const {
    auto cachedValue = cache::Cache::get().getValueByKey( getId() );
    if( !cachedValue ) {
        return createDefaultValue();
    }
    return cachedValue;
}

std::unique_ptr<::common::IValue> BaseInfo::getValueCache() const {
    return ::cache::Cache::get().getValueByKey( getId() );
}

void BaseInfo::setValue( std::unique_ptr<::common::IValue> newValue ) {
    ::cache::Cache::get().setValueByKey( getId(), std::move( newValue ) );
}

std::tuple<common::IValue::UPtr, int, std::string> BaseInfo::createValue( const float percent ) const {
    using namespace common::resources;

    if( !isMinMax() ) {
        return std::make_tuple( nullptr, errors::InvalidParams, errors::data::WrongType );
    }

    auto ival = getValue();
    if( !ival ) {
        return std::make_tuple( nullptr, errors::InternalError, errors::data::EzloInternaleError );
    }

    DeltaValueSetter valueSetter( percent, getMinMax() );
    ival->accept( valueSetter );
    return std::make_tuple( errors::NotError == valueSetter.errorCode() ? std::move( ival ) : nullptr,
                            valueSetter.errorCode(),
                            std::move( valueSetter.errorMessage() ) );
}

common::IValue::UPtr BaseInfo::castToType( common::IValue::UPtr&& ival, ItemValueType type ) const {
    switch( type ) {
    case ItemValueType::Int:
        return common::iValueCast<common::IntValue>( std::move( ival ) );

    case ItemValueType::Bool:
        return common::iValueCast<common::BoolValue>( std::move( ival ) );

    case ItemValueType::Float:
        return common::iValueCast<common::FloatValue>( std::move( ival ) );

    case ItemValueType::CameraStream:
        return {};

    case ItemValueType::Illuminance:
    case ItemValueType::Pressure:
    case ItemValueType::Substance_amount:
    case ItemValueType::Power:
    case ItemValueType::Humidity:
    case ItemValueType::Velocity:
    case ItemValueType::Acceleration:
    case ItemValueType::Direction:
    case ItemValueType::General_purpose:
    case ItemValueType::Acidity:
    case ItemValueType::Electric_potential:
    case ItemValueType::Electric_current:
    case ItemValueType::Force:
    case ItemValueType::Irradiance:
    case ItemValueType::Precipitation:
    case ItemValueType::Length:
    case ItemValueType::Mass:
    case ItemValueType::Volume_flow:
    case ItemValueType::Volume:
    case ItemValueType::Angle:
    case ItemValueType::Frequency:
    case ItemValueType::Seismic_intensity:
    case ItemValueType::Seismic_magnitude:
    case ItemValueType::Ultraviolet:
    case ItemValueType::Electrical_resistance:
    case ItemValueType::Electrical_conductivity:
    case ItemValueType::Loudness:
    case ItemValueType::Moisture:
    case ItemValueType::Time:
    case ItemValueType::Radon_concentration:
    case ItemValueType::Blood_pressure:
    case ItemValueType::Energy:
    case ItemValueType::Amount_of_useful_energy:
    case ItemValueType::Rf_signal_strength:
    case ItemValueType::Temperature:
    case ItemValueType::Kilo_volt_ampere_hour:
    case ItemValueType::Reactive_power_instant:
    case ItemValueType::Reactive_power_consumption: {
        if( !ival ) {
            FUNC_LOGE( "Wrong value" );
            return {};
        }

        std::unique_ptr<common::ScalebleValue> value;

        // is necessary to do iValueCast from table to ScalebleValue
        auto tblIval = common::iValueCast<common::TableValue>( std::move( ival->clone() ) );
        if( tblIval ) {
            if( !tblIval->isKeyExist( tokens::ValueType ) ) {
                tblIval->insert( tokens::ValueType,
                                 std::make_unique<common::StringValue>( toString( getValueType() ) ) );
            }
            value = common::iValueCast<common::ScalebleValue>( std::move( tblIval ) );
        }
        else {
            value = common::iValueCast<common::ScalebleValue>( std::move( ival->clone() ) );
        }

        if( !value ) {
            value.reset();

            float number = 0.0;
            auto fValue = common::iValueCast<common::FloatValue>( std::move( ival->clone() ) );
            if( !fValue ) {
                auto iValue = common::iValueCast<common::IntValue>( std::move( ival->clone() ) );
                if( !iValue ) {
                    FUNC_LOGW( "Wrong value" );
                    return {};
                }
                number = iValue->get();
            }
            else {
                number = fValue->get();
            }
            value = std::make_unique<common::ScalebleValue>(
                number,
                core::HubSettingsInfo::temperatureScaleToStr( core::HubSettings::get().getTemperatureScale() ) );
        }

        if( value ) {
            std::string scale = value->getScale();
            if( scale.empty() ) {
                scale = tokens::Unknown;  // #TODO Now we don't store scale for scaleable types. Use placeholder instead
            }
            common::ScalesConverterVisitor visitor( scale, common::ScalesConverterVisitor::Precision );
            value->accept( visitor );
            return value;
        }
        FUNC_LOGW( "Wrong value" );
        return {};
    }

    case ItemValueType::RGB:
        return common::iValueCast<common::RgbValue>( std::move( ival ) );

    case ItemValueType::Usercode:
        return common::iValueCast<common::UserCodeValue>( std::move( ival ) );

    case ItemValueType::String:
    case ItemValueType::Token:
        return common::iValueCast<common::StringValue>( std::move( ival ) );

    case ItemValueType::Button_state: {
        auto value = common::iValueCast<common::TableValue>( ival.get() );
        if( value ) {
            auto buttonNumber = ( *value )[tokens::ButtonNumber];
            if( buttonNumber && buttonNumber->getId() != common::IValue::IValueId::IntVal ) {
                auto castedVal = common::iValueCast<common::IntValue>( value->remove( tokens::ButtonNumber ) );
                if( !castedVal ) {
                    return nullptr;
                }
                value->insert( tokens::ButtonNumber, std::move( castedVal ) );
            }
            auto buttonState = ( *value )[values::Button_state];
            if( buttonState && buttonState->getId() != common::IValue::IValueId::StringVal ) {
                auto castedVal = common::iValueCast<common::StringValue>( value->remove( values::Button_state ) );
                if( !castedVal ) {
                    return nullptr;
                }
                value->insert( values::Button_state, std::move( castedVal ) );
            }
            return std::move( ival );
        }
        return common::iValueCast<common::StringValue>( std::move( ival ) );
    }
    case ItemValueType::Action:
    case ItemValueType::Zwave_configuration:
    case ItemValueType::Dictionary:
    case ItemValueType::User_lock_operation:
        return common::iValueCast<common::TableValue>( std::move( ival ) );
    case ItemValueType::Array:
        return common::iValueCast<common::ArrayValue>( std::move( ival ) );

    default:
        return {};
    };
}

common::IValue::UPtr BaseInfo::castToItemType( common::IValue::UPtr&& ival ) const {
    return castToType( std::move( ival ), m_valueType );
}

common::IValue::UPtr BaseInfo::createDefaultValue() const {
    switch( m_valueType ) {
    case ItemValueType::Int:
        return std::make_unique<common::IntValue>();

    case ItemValueType::Bool:
        return std::make_unique<common::BoolValue>();

    case ItemValueType::Float:
        return std::make_unique<common::FloatValue>();

    case ItemValueType::Illuminance:
    case ItemValueType::Pressure:
    case ItemValueType::Substance_amount:
    case ItemValueType::Power:
    case ItemValueType::Humidity:
    case ItemValueType::Velocity:
    case ItemValueType::Acceleration:
    case ItemValueType::Direction:
    case ItemValueType::General_purpose:
    case ItemValueType::Acidity:
    case ItemValueType::Electric_potential:
    case ItemValueType::Electric_current:
    case ItemValueType::Force:
    case ItemValueType::Irradiance:
    case ItemValueType::Precipitation:
    case ItemValueType::Length:
    case ItemValueType::Mass:
    case ItemValueType::Volume_flow:
    case ItemValueType::Volume:
    case ItemValueType::Angle:
    case ItemValueType::Frequency:
    case ItemValueType::Seismic_intensity:
    case ItemValueType::Seismic_magnitude:
    case ItemValueType::Ultraviolet:
    case ItemValueType::Electrical_resistance:
    case ItemValueType::Electrical_conductivity:
    case ItemValueType::Loudness:
    case ItemValueType::Moisture:
    case ItemValueType::Time:
    case ItemValueType::Radon_concentration:
    case ItemValueType::Blood_pressure:
    case ItemValueType::Energy:
    case ItemValueType::Amount_of_useful_energy:
    case ItemValueType::Rf_signal_strength:
    case ItemValueType::Temperature:
    case ItemValueType::Kilo_volt_ampere_hour:
    case ItemValueType::Reactive_power_instant:
    case ItemValueType::Reactive_power_consumption: {
        auto ival = std::make_unique<common::ScalebleValue>();
        return ival;
    }
    case ItemValueType::RGB:
        return std::make_unique<common::RgbValue>();

    case ItemValueType::String:
    case ItemValueType::Token:
        return std::make_unique<common::StringValue>();

    case ItemValueType::Usercode:
    case ItemValueType::Button_state:
    case ItemValueType::Action:
    case ItemValueType::Zwave_configuration:
    case ItemValueType::Dictionary:
        return std::make_unique<common::TableValue>();
    case ItemValueType::Array:
        return std::make_unique<common::ArrayValue>();
    case ItemValueType::User_lock_operation: {
        FUNC_LOGD( "User_lock_operation" );
        auto table = std::make_unique<common::TableValue>();
        table->insert( "action", std::make_unique<common::StringValue>( "unknown" ) );
        table->insert( "userId", std::make_unique<common::IntValue>( -1 ) );
        return table;
    }
    default:
        return {};
    };
}

const char* toString( ItemValueType type ) {
    using namespace common::resources;

    switch( type ) {
    case ItemValueType::Int:
        return values::Int;

    case ItemValueType::Bool:
        return values::Bool;

    case ItemValueType::Float:
        return values::Float;

    case ItemValueType::RGB:
        return values::RGB;

    case ItemValueType::Usercode:
        return values::Usercode;

    case ItemValueType::String:
        return values::String;

    case ItemValueType::Token:
        return values::Token;

    case ItemValueType::CameraStream:
        return values::CameraStream;

    case ItemValueType::Illuminance:
        return values::Illuminance;

    case ItemValueType::Pressure:
        return values::Pressure;

    case ItemValueType::Substance_amount:
        return values::Substance_amount;

    case ItemValueType::Power:
        return values::Power;

    case ItemValueType::Humidity:
        return values::Humidity;

    case ItemValueType::Velocity:
        return values::Velocity;

    case ItemValueType::Acceleration:
        return values::Acceleration;

    case ItemValueType::Direction:
        return values::Direction;

    case ItemValueType::General_purpose:
        return values::General_purpose;

    case ItemValueType::Acidity:
        return values::Acidity;

    case ItemValueType::Electric_potential:
        return values::Electric_potential;

    case ItemValueType::Electric_current:
        return values::Electric_current;

    case ItemValueType::Force:
        return values::Force;

    case ItemValueType::Irradiance:
        return values::Irradiance;

    case ItemValueType::Precipitation:
        return values::Precipitation;

    case ItemValueType::Length:
        return values::Length;

    case ItemValueType::Mass:
        return values::Mass;

    case ItemValueType::Volume_flow:
        return values::Volume_flow;

    case ItemValueType::Volume:
        return values::Volume;

    case ItemValueType::Angle:
        return values::Angle;

    case ItemValueType::Frequency:
        return values::Frequency;

    case ItemValueType::Seismic_intensity:
        return values::Seismic_intensity;

    case ItemValueType::Seismic_magnitude:
        return values::Seismic_magnitude;

    case ItemValueType::Ultraviolet:
        return values::Ultraviolet;

    case ItemValueType::Electrical_resistance:
        return values::Electrical_resistance;

    case ItemValueType::Electrical_conductivity:
        return values::Electrical_conductivity;

    case ItemValueType::Loudness:
        return values::Loudness;

    case ItemValueType::Moisture:
        return values::Moisture;

    case ItemValueType::Time:
        return values::Time;

    case ItemValueType::Radon_concentration:
        return values::Radon_concentration;

    case ItemValueType::Blood_pressure:
        return values::Blood_pressure;

    case ItemValueType::Energy:
        return values::Energy;

    case ItemValueType::Rf_signal_strength:
        return values::Rf_signal_strength;

    case ItemValueType::Temperature:
        return values::Temperature;

    case ItemValueType::Button_state:
        return values::Button_state;

    case ItemValueType::Array:
        return values::Array;

    case ItemValueType::Action:
        return values::Action;

    case ItemValueType::Zwave_configuration:
        return values::Zwave_configuration;

    case ItemValueType::Dictionary:
        return values::Dictionary;

    case ItemValueType::User_lock_operation:
        return values::User_lock_operation;

    case ItemValueType::Amount_of_useful_energy:
        return values::AmountOfUsefulEnergy;

    case ItemValueType::Kilo_volt_ampere_hour:
        return values::Kilo_volt_ampere_hour;

    case ItemValueType::Reactive_power_instant:
        return values::Reactive_power_instant;

    case ItemValueType::Reactive_power_consumption:
        return values::AmountOfUsefulEnergy;

    default:
        return tokens::Unknown;
    }
}

ItemValueType toItemValueType( const char* typeName ) {
    using namespace common::resources;

    if( strcmp( values::Int, typeName ) == 0 ) {
        return ItemValueType::Int;
    }
    else if( strcmp( values::Bool, typeName ) == 0 ) {
        return ItemValueType::Bool;
    }
    else if( strcmp( values::Float, typeName ) == 0 ) {
        return ItemValueType::Float;
    }
    else if( strcmp( values::RGB, typeName ) == 0 ) {
        return ItemValueType::RGB;
    }
    else if( strcmp( values::Usercode, typeName ) == 0 ) {
        return ItemValueType::Usercode;
    }
    else if( strcmp( values::String, typeName ) == 0 ) {
        return ItemValueType::String;
    }
    else if( strcmp( values::Token, typeName ) == 0 ) {
        return ItemValueType::Token;
    }
    else if( strcmp( values::CameraStream, typeName ) == 0 ) {
        return ItemValueType::CameraStream;
    }
    else if( strcmp( values::Illuminance, typeName ) == 0 ) {
        return ItemValueType::Illuminance;
    }
    else if( strcmp( values::Pressure, typeName ) == 0 ) {
        return ItemValueType::Pressure;
    }
    else if( strcmp( values::Substance_amount, typeName ) == 0 ) {
        return ItemValueType::Substance_amount;
    }
    else if( strcmp( values::Power, typeName ) == 0 ) {
        return ItemValueType::Power;
    }
    else if( strcmp( values::Humidity, typeName ) == 0 ) {
        return ItemValueType::Humidity;
    }
    else if( strcmp( values::Velocity, typeName ) == 0 ) {
        return ItemValueType::Velocity;
    }
    else if( strcmp( values::Acceleration, typeName ) == 0 ) {
        return ItemValueType::Acceleration;
    }
    else if( strcmp( values::Direction, typeName ) == 0 ) {
        return ItemValueType::Direction;
    }
    else if( strcmp( values::General_purpose, typeName ) == 0 ) {
        return ItemValueType::General_purpose;
    }
    else if( strcmp( values::Acidity, typeName ) == 0 ) {
        return ItemValueType::Acidity;
    }
    else if( strcmp( values::Electric_potential, typeName ) == 0 ) {
        return ItemValueType::Electric_potential;
    }
    else if( strcmp( values::Electric_current, typeName ) == 0 ) {
        return ItemValueType::Electric_current;
    }
    else if( strcmp( values::Force, typeName ) == 0 ) {
        return ItemValueType::Force;
    }
    else if( strcmp( values::Irradiance, typeName ) == 0 ) {
        return ItemValueType::Irradiance;
    }
    else if( strcmp( values::Precipitation, typeName ) == 0 ) {
        return ItemValueType::Precipitation;
    }
    else if( strcmp( values::Length, typeName ) == 0 ) {
        return ItemValueType::Length;
    }
    else if( strcmp( values::Mass, typeName ) == 0 ) {
        return ItemValueType::Mass;
    }
    else if( strcmp( values::Volume_flow, typeName ) == 0 ) {
        return ItemValueType::Volume_flow;
    }
    else if( strcmp( values::Volume, typeName ) == 0 ) {
        return ItemValueType::Volume;
    }
    else if( strcmp( values::Angle, typeName ) == 0 ) {
        return ItemValueType::Angle;
    }
    else if( strcmp( values::Frequency, typeName ) == 0 ) {
        return ItemValueType::Frequency;
    }
    else if( strcmp( values::Seismic_intensity, typeName ) == 0 ) {
        return ItemValueType::Seismic_intensity;
    }
    else if( strcmp( values::Seismic_magnitude, typeName ) == 0 ) {
        return ItemValueType::Seismic_magnitude;
    }
    else if( strcmp( values::Ultraviolet, typeName ) == 0 ) {
        return ItemValueType::Ultraviolet;
    }
    else if( strcmp( values::Electrical_resistance, typeName ) == 0 ) {
        return ItemValueType::Electrical_resistance;
    }
    else if( strcmp( values::Electrical_conductivity, typeName ) == 0 ) {
        return ItemValueType::Electrical_conductivity;
    }
    else if( strcmp( values::Loudness, typeName ) == 0 ) {
        return ItemValueType::Loudness;
    }
    else if( strcmp( values::Moisture, typeName ) == 0 ) {
        return ItemValueType::Moisture;
    }
    else if( strcmp( values::Time, typeName ) == 0 ) {
        return ItemValueType::Time;
    }
    else if( strcmp( values::Radon_concentration, typeName ) == 0 ) {
        return ItemValueType::Radon_concentration;
    }
    else if( strcmp( values::Blood_pressure, typeName ) == 0 ) {
        return ItemValueType::Blood_pressure;
    }
    else if( strcmp( values::Energy, typeName ) == 0 ) {
        return ItemValueType::Energy;
    }
    else if( strcmp( values::Rf_signal_strength, typeName ) == 0 ) {
        return ItemValueType::Rf_signal_strength;
    }
    else if( strcmp( values::Temperature, typeName ) == 0 ) {
        return ItemValueType::Temperature;
    }
    else if( strcmp( values::Button_state, typeName ) == 0 ) {
        return ItemValueType::Button_state;
    }
    else if( strcmp( values::Action, typeName ) == 0 ) {
        return ItemValueType::Action;
    }
    else if( strcmp( values::Zwave_configuration, typeName ) == 0 ) {
        return ItemValueType::Zwave_configuration;
    }
    else if( strncmp( values::Dictionary, typeName, strlen( values::Dictionary ) ) == 0 ) {
        return ItemValueType::Dictionary;
    }
    else if( strcmp( values::User_lock_operation, typeName ) == 0 ) {
        return ItemValueType::User_lock_operation;
    }
    else if( strncmp( values::Array, typeName, strlen( values::Array ) ) == 0 ) {
        return ItemValueType::Array;
    }
    else if( strcmp( values::AmountOfUsefulEnergy, typeName ) == 0 ) {
        return ItemValueType::Amount_of_useful_energy;
    }
    else if( strcmp( values::Kilo_volt_ampere_hour, typeName ) == 0 ) {
        return ItemValueType::Kilo_volt_ampere_hour;
    }
    else if( strcmp( values::Reactive_power_instant, typeName ) == 0 ) {
        return ItemValueType::Reactive_power_instant;
    }
    else if( strcmp( values::Reactive_power_consumption, typeName ) == 0 ) {
        return ItemValueType::Reactive_power_consumption;
    }
    else {
        FUNC_LOGW( "Unknown item value type: %s", typeName );
    }

    return ItemValueType::NoValue;
}

}  // namespace gateways
