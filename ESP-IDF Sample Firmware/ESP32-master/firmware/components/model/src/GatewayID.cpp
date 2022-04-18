
#include <GatewayID.h>
#include <string.h>

#include <CommonResources.h>

#include <type_traits>

namespace gateways {

const char* toString( GatewayID type ) {
    switch( type ) {
    case GatewayID::ZWave:
        return common::resources::gateway_names::ZWave;
#ifdef CONFIG_CLOUD_DEVICES
    case GatewayID::Cloud:
        return common::resources::gateway_names::Cloud;
#endif
#if CONFIG_BOARD_TYPE_PLUG_HUB
    case GatewayID::PlugHub:
        return common::resources::gateway_names::PlugHub;
#endif
#if CONFIG_BOARD_TYPE_WATER_VALVE
    case GatewayID::WaterValve:
        return common::resources::gateway_names::WaterValve;
#endif
    default:
        break;
    };

    return common::resources::tokens::Unknown;
}

GatewayID toGatewayID( const char* name ) {
    GatewayID result = GatewayID::First;

    for( ; result != GatewayID::Undefined; ) {
        if( strcasecmp( toString( result ), name ) == 0 ) {
            return result;
        }

        result = static_cast<GatewayID>( static_cast<std::underlying_type<GatewayID>::type>( result ) + 1 );
    }

    return GatewayID::Undefined;
}

char toDevicePrefix( GatewayID type ) {
    switch( type ) {
    case GatewayID::ZWave:
        return 'Z';
#ifdef CONFIG_CLOUD_DEVICES
    case GatewayID::Cloud:
        return 'C';
#endif
#if CONFIG_BOARD_TYPE_PLUG_HUB
    case GatewayID::PlugHub:
        return 'P';
#endif
#if CONFIG_BOARD_TYPE_WATER_VALVE
    case GatewayID::WaterValve:
        return 'W';
#endif
    default:
        break;
    };

    return 'U';
}

GatewayID toGatewayIDByDevicePrefix( char devicePrefix ) {
    GatewayID result = GatewayID::First;

    for( ; result != GatewayID::Undefined; ) {
        if( devicePrefix == toDevicePrefix( result ) ) {
            return result;
        }

        result = static_cast<GatewayID>( static_cast<std::underlying_type<GatewayID>::type>( result ) + 1 );
    }

    return GatewayID::Undefined;
}

}  // namespace gateways
