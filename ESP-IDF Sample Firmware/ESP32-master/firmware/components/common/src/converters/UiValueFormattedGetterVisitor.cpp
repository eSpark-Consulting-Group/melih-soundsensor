#include "converters/UiValueFormattedGetterVisitor.hpp"

#include "CommonResources.h"
#include "DebugUtils.h"
#include "EspLogTags.h"
#include <UnitConverter.h>

#include <sstream>

namespace common {

UiValueFormattedGetterVisitor::UiValueFormattedGetterVisitor() {}

UiValueFormattedGetterVisitor::~UiValueFormattedGetterVisitor() = default;

const std::string& UiValueFormattedGetterVisitor::getValue( const IValue& value ) {
    m_result.clear();

    value.accept( *this );

    return m_result;
}

void UiValueFormattedGetterVisitor::visit( const FloatValue& value ) {
    m_result = std::to_string( value.get() );
}

void UiValueFormattedGetterVisitor::visit( const ScalebleValue& value ) {
    m_result = std::to_string( value.get() );
}

void UiValueFormattedGetterVisitor::visit( const IntValue& value ) {
    m_result = std::to_string( value.get() );
}

void UiValueFormattedGetterVisitor::visit( const Int64Value& value ) {
    m_result = std::to_string( value.get() );
}

void UiValueFormattedGetterVisitor::visit( const BoolValue& value ) {
    m_result = value.get() ? "true" : "false";
}

void UiValueFormattedGetterVisitor::visit( const StringValue& value ) {
    m_result = value.get();
}

void UiValueFormattedGetterVisitor::visit( const TokenValue& value ) {
    m_result = value.get();
}

void UiValueFormattedGetterVisitor::visit( const RgbValue& value ) {
    m_result = rgbToFormattedString( value.getComponentValue( RgbValue::Color::Red ),
                                     value.getComponentValue( RgbValue::Color::Green ),
                                     value.getComponentValue( RgbValue::Color::Blue ) );
}

void UiValueFormattedGetterVisitor::visit( const NotificationValue& value ) {}

void UiValueFormattedGetterVisitor::visit( const UserCodeValue& value ) {}

void UiValueFormattedGetterVisitor::visit( const ArrayValue& value ) {}

void UiValueFormattedGetterVisitor::visit( const TableValue& value ) {}

std::string UiValueFormattedGetterVisitor::rgbToFormattedString( uint8_t red, uint8_t green, uint8_t blue ) {
    std::ostringstream oss;
    oss << '#';
    oss.fill( '0' );
    oss.width( 6 );
    oss << std::uppercase << std::hex << ( ( red << 16 ) | ( green << 8 ) | blue );
    return oss.str();
}

}  // namespace common
