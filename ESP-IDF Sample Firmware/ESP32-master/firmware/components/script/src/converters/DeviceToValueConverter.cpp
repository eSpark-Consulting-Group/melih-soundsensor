#include "converters/DeviceToValueConverter.h"

#include <znet/converters/zwave_plus_icon_types_converter.hpp>
#include <znet/converters/generic_types_converter.hpp>
#include <znet/converters/specific_types_converter.hpp>
#include <znet/converters/security_modes_converter.hpp>

#include "values/IntValue.h"
#include "values/BoolValue.h"
#include "values/StringValue.h"
#include "values/ArrayValue.h"
#include "values/TableValue.h"

#include <CommonResources.h>

namespace lua {
namespace module {
namespace converters {
namespace zwave {

DeviceToValueConverter::DeviceToValueConverter(::common::TableValue& value )
: m_value( value ) {}

void DeviceToValueConverter::operator()( const ::gateways::zwave::devices::Device& device ) {
    auto security = std::make_unique<::common::ArrayValue>();
    security->push_back( std::make_unique<::common::StringValue>(
        ::znet::security_modes_converter::typeToString( device.getSecurity() ) ) );

    auto securityClassStarted = std::make_unique<::common::ArrayValue>();
    securityClassStarted->push_back( std::make_unique<::common::StringValue>(
        ::znet::security_modes_converter::typeToString( device.getSecurityClassStarted() ) ) );

    std::array<char, 64> id;
    ::snprintf( id.data(), id.size(), "%d", device.getId() );
    m_value.insert( common::resources::lua_events::Id, std::make_unique<::common::StringValue>( id.data() ) );
    m_value.insert( common::resources::lua_events::Security, std::move( security ) );
    m_value.insert( common::resources::lua_events::securityClassStarted, std::move( securityClassStarted ) );
    m_value.insert( common::resources::lua_events::ManufacturerId,
                    std::make_unique<::common::IntValue>( device.getManufacturerId() ) );
    m_value.insert( common::resources::lua_events::ProductType,
                    std::make_unique<::common::IntValue>( device.getProductType() ) );
    m_value.insert( common::resources::lua_events::ProductId,
                    std::make_unique<::common::IntValue>( device.getProductId() ) );
    m_value.insert( common::resources::lua_events::IsBatteryPowered,
                    std::make_unique<::common::BoolValue>( device.isBatteryPowered() ) );
    m_value.insert(::common::resources::lua_events::IsSmartInclude,
                   std::make_unique<::common::BoolValue>( device.isSmartStart() ) );
    m_value.insert(::common::resources::lua_events::IsRediscoverRequired,
                   std::make_unique<::common::BoolValue>( device.isRediscoverRequired() ) );

    if( auto iconType = ::znet::zwave_plus_icon_types_converter::typeToString(
            static_cast<::znet::ZwavePlusIconType>( device.getZwavePlusIconType() ) ) ) {
        m_value.insert( common::resources::lua_events::IconType, std::make_unique<::common::StringValue>( iconType ) );
    }
    if( auto genericType = ::znet::generic_types_converter::typeToString(
            static_cast<::znet::GenericType>( device.getGenericType() ) ) ) {
        m_value.insert( common::resources::lua_events::GenericType,
                        std::make_unique<::common::StringValue>( genericType ) );
    }
    if( auto specificType = ::znet::specific_types_converter::typeToString(
            static_cast<::znet::GenericType>( device.getGenericType() ), device.getSpecificType() ) ) {
        m_value.insert( common::resources::lua_events::SpecificType,
                        std::make_unique<::common::StringValue>( specificType ) );
    }

    auto wakeUpInterval = device.getWakeUpInterval();
    if( wakeUpInterval ) {
        m_value.insert( common::resources::lua_events::WakeUpInterval,
                        std::make_unique<::common::IntValue>( *wakeUpInterval ) );
    }
}

}  // namespace zwave
}  // namespace converters
}  // namespace module
}  // namespace lua
