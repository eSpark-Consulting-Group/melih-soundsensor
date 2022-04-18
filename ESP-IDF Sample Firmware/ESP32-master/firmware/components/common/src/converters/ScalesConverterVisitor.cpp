#include "converters/ScalesConverterVisitor.hpp"

#include "CommonResources.h"
#include "DebugUtils.h"
#include "EspLogTags.h"
#include <UnitConverter.h>

namespace common {

ScalesConverterBaseVisitor::ScalesConverterBaseVisitor( std::string _dstScale )
: dstScale( _dstScale ) {}

void ScalesConverterBaseVisitor::visit( common::ScalebleValue& value ) {
    std::string srcScale = value.getScale();
    if( !srcScale.empty() && !dstScale.empty() && srcScale != dstScale ) {
        if( strncmp( dstScale.c_str(), common::resources::scales::Celsius, 7 ) == 0 &&
            strncmp( srcScale.c_str(), common::resources::scales::Fahrenheit, 10 ) == 0 ) {
            value.set( common::UnitConverter::fahrenheitToCelsius( value.get() ) );
            value.setScale( dstScale );
        }
        else if( strncmp( dstScale.c_str(), common::resources::scales::Fahrenheit, 10 ) == 0 &&
                 strncmp( srcScale.c_str(), common::resources::scales::Celsius, 7 ) == 0 ) {
            value.set( common::UnitConverter::celsiusToFahrenheit( value.get() ) );
            value.setScale( dstScale );
        }
    }
}

const uint8_t ScalesConverterVisitor::Precision =
    2;  // TODO  - temporary. Use the Hub's configuration value subsequently

ScalesConverterVisitor::ScalesConverterVisitor( std::string _dstScale, uint8_t _precision )
: ScalesConverterBaseVisitor( _dstScale )
, precision( _precision ) {}

void ScalesConverterVisitor::visit( common::ScalebleValue& value ) {
    std::string srcScale = value.getScale();
    if( srcScale.empty() ) {
        FUNC_LOGE( "Tries to cast an empty source scale" );
        return;
    }
    if( dstScale.empty() ) {
        FUNC_LOGE( "Tries to cast an empty destination scale" );
        return;
    }

    if( srcScale != dstScale ) {
        if( strncmp( dstScale.c_str(), common::resources::scales::Celsius, 7 ) == 0 &&
            strncmp( srcScale.c_str(), common::resources::scales::Fahrenheit, 10 ) == 0 ) {
            float res =
                static_cast<float>( static_cast<int>( common::UnitConverter::fahrenheitToCelsius( value.get() ) *
                                                          common::UnitConverter::getPower( precision ) +
                                                      0.5 ) ) /
                common::UnitConverter::getPower( precision );
            if( value.getRelative() ) {
                res -= static_cast<float>( static_cast<int>( common::UnitConverter::fahrenheitToCelsius( 0.0 ) *
                                                                 common::UnitConverter::getPower( precision ) +
                                                             0.5 ) ) /
                       common::UnitConverter::getPower( precision );
            }
            value.set( res );
            value.setScale( dstScale );
        }
        else if( strncmp( dstScale.c_str(), common::resources::scales::Fahrenheit, 10 ) == 0 &&
                 strncmp( srcScale.c_str(), common::resources::scales::Celsius, 7 ) == 0 ) {
            float res =
                static_cast<float>( static_cast<int>( common::UnitConverter::celsiusToFahrenheit( value.get() ) *
                                                          common::UnitConverter::getPower( precision ) +
                                                      0.5 ) ) /
                common::UnitConverter::getPower( precision );
            if( value.getRelative() ) {
                res -= static_cast<float>( static_cast<int>( common::UnitConverter::celsiusToFahrenheit( 0.0 ) *
                                                                 common::UnitConverter::getPower( precision ) +
                                                             0.5 ) ) /
                       common::UnitConverter::getPower( precision );
            }
            value.set( res );
            value.setScale( dstScale );
        }
    }
}

ScalesConverterFromFloatVisitor::ScalesConverterFromFloatVisitor( std::string _srcScale,
                                                                  std::string _dstScale,
                                                                  uint8_t _precision,
                                                                  float _srcValue )
: ScalesConverterVisitor( _dstScale, _precision )
, m_srcScale( _srcScale )
, m_srcValue( _srcValue ) {}

void ScalesConverterFromFloatVisitor::visit( common::ScalebleValue& value ) {
    if( m_srcScale.empty() ) {
        FUNC_LOGE( "Tries to cast an empty source scale" );
        return;
    }
    if( dstScale.empty() ) {
        FUNC_LOGE( "Tries to cast an empty destination scale" );
        return;
    }
    if( m_srcScale != dstScale ) {
        if( strncmp( dstScale.c_str(), common::resources::scales::Celsius, 7 ) == 0 &&
            strncmp( m_srcScale.c_str(), common::resources::scales::Fahrenheit, 10 ) == 0 ) {
            auto res = static_cast<float>( static_cast<int>( common::UnitConverter::fahrenheitToCelsius( m_srcValue ) *
                                                                 common::UnitConverter::getPower( precision ) +
                                                             0.5 ) ) /
                       common::UnitConverter::getPower( precision );
            if( value.getRelative() ) {
                res -= static_cast<float>( static_cast<int>( common::UnitConverter::fahrenheitToCelsius( 0.0 ) *
                                                                 common::UnitConverter::getPower( precision ) +
                                                             0.5 ) ) /
                       common::UnitConverter::getPower( precision );
            }
            value.set( res );
            value.setScale( dstScale );
        }
        else if( strncmp( dstScale.c_str(), common::resources::scales::Fahrenheit, 10 ) == 0 &&
                 strncmp( m_srcScale.c_str(), common::resources::scales::Celsius, 7 ) == 0 ) {
            auto res = static_cast<float>( static_cast<int>( common::UnitConverter::celsiusToFahrenheit( m_srcValue ) *
                                                                 common::UnitConverter::getPower( precision ) +
                                                             0.5 ) ) /
                       common::UnitConverter::getPower( precision );
            if( value.getRelative() ) {
                res -= static_cast<float>( static_cast<int>( common::UnitConverter::celsiusToFahrenheit( 0.0 ) *
                                                                 common::UnitConverter::getPower( precision ) +
                                                             0.5 ) ) /
                       common::UnitConverter::getPower( precision );
            }
            value.set( res );
            value.setScale( dstScale );
        }
    }
    else {
        value.set( m_srcValue );  // remain value
    }
}

}  // namespace common
