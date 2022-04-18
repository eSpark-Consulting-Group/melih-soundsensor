
#pragma once

#include <sdkconfig.h>
#include <type_traits>
#include <functional>

namespace gateways {

enum class GatewayID {
    First = 0,

    ZWave = First,
#ifdef CONFIG_CLOUD_DEVICES
    Cloud,
#endif
#if CONFIG_BOARD_TYPE_PLUG_HUB
    PlugHub,
#elif CONFIG_BOARD_TYPE_WATER_VALVE
    WaterValve,
#endif
    Undefined
};

const char* toString( GatewayID type );
char toDevicePrefix( GatewayID type );
GatewayID toGatewayIDByDevicePrefix( char devicePrefix );
GatewayID toGatewayID( const char* name );

inline GatewayID& operator++( GatewayID& id ) {
    id = static_cast<gateways::GatewayID>( static_cast<std::underlying_type<gateways::GatewayID>::type>( id ) + 1 );
    return id;
}

}  // namespace gateways
