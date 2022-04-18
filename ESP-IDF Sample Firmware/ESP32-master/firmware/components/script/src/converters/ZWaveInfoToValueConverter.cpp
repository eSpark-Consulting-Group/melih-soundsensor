#include "converters/ZWaveInfoToValueConverter.h"

#include "ZWaveGateway.h"

#include "values/IntValue.h"
#include "values/BoolValue.h"
#include "values/StringValue.h"
#include "values/TableValue.h"

#include <CommonResources.h>

namespace lua {
namespace module {
namespace converters {
namespace zwave {

ZWaveInfoToValueConverter::ZWaveInfoToValueConverter(::common::TableValue& value )
: m_value( value ) {}

void ZWaveInfoToValueConverter::operator()() {
    auto info = gateways::zwave::ZNet::get().getCommonInfo();

    m_value.insert( common::resources::gateway_zwave::Module,
                    std::make_unique<::common::StringValue>( info->m_module ) );
    m_value.insert( common::resources::gateway_zwave::ModuleFirmwareVersion,
                    std::make_unique<::common::StringValue>( info->m_module_firmware_version ) );
    m_value.insert( common::resources::gateway_zwave::Locale,
                    std::make_unique<::common::StringValue>( info->m_locale ) );
    m_value.insert( common::resources::gateway_zwave::Home_id,
                    std::make_unique<::common::StringValue>( info->m_home_id ) );
    m_value.insert( common::resources::gateway_zwave::Node_id,
                    std::make_unique<::common::IntValue>( info->m_node_id ) );
    m_value.insert( common::resources::gateway_zwave::Port, std::make_unique<::common::StringValue>( "" ) );

    auto poll = std::make_unique<::common::TableValue>();
    poll->insert( common::resources::gateway_zwave::Powered,
                  std::make_unique<::common::IntValue>( info->m_pole.m_powered ) );
    poll->insert( common::resources::gateway_zwave::Battery,
                  std::make_unique<::common::IntValue>( info->m_pole.m_battery ) );
    m_value.insert( common::resources::gateway_zwave::Poll, std::move( poll ) );

    auto roles = std::make_unique<::common::TableValue>();
    roles->insert( common::resources::gateway_zwave::Sis,
                   std::make_unique<::common::BoolValue>( static_cast<bool>( info->m_roles.m_sis ) ) );
    roles->insert( common::resources::gateway_zwave::Primary,
                   std::make_unique<::common::BoolValue>( static_cast<bool>( info->m_roles.m_primary ) ) );
    roles->insert( common::resources::gateway_zwave::Secondary,
                   std::make_unique<::common::BoolValue>( static_cast<bool>( info->m_roles.m_secondary ) ) );
    roles->insert( common::resources::gateway_zwave::Static,
                   std::make_unique<::common::BoolValue>( static_cast<bool>( info->m_roles.m_static ) ) );
    roles->insert( common::resources::gateway_zwave::Inclusion,
                   std::make_unique<::common::BoolValue>( static_cast<bool>( info->m_roles.m_inclusion ) ) );

    m_value.insert( common::resources::gateway_zwave::Roles, std::move( roles ) );
}

}  // namespace zwave
}  // namespace converters
}  // namespace module
}  // namespace lua
