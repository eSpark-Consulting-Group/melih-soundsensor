
#include <IGateway.h>
#include <sdkconfig.h>

#include <ZWaveGateway.h>
#include <CloudGateway.h>
#if CONFIG_BOARD_TYPE_PLUG_HUB
#include <PlugHubGateway.h>
#endif
#if CONFIG_BOARD_TYPE_WATER_VALVE
#include <WaterValveGateway.h>
#endif
#include <JsonStreamGenerator.h>
#include <CommonResources.h>

#include <IValue.h>
#include <values/TableValue.h>

#include <Ezlo.h>
#include <GatewayCommand.h>

#include <DebugUtils.h>

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace gateways {

IGateway* get( const GatewayID id ) {
    switch( id ) {
    case GatewayID::ZWave:
        return &zwave::Gateway::get();
#ifdef CONFIG_CLOUD_DEVICES
    case GatewayID::Cloud:
        return &cloud::CloudGateway::get();
#endif
#if CONFIG_BOARD_TYPE_PLUG_HUB
    case GatewayID::PlugHub:
        return &plug_hub::Gateway::get();
#endif
#if CONFIG_BOARD_TYPE_WATER_VALVE
    case GatewayID::WaterValve:
        return &water_valve::Gateway::get();
#endif
    default:
        break;
    };

    return nullptr;
}

IGateway* getByName( const char* name ) {
    for( auto getawayId = gateways::GatewayID::First; getawayId < gateways::GatewayID::Undefined; ++getawayId ) {
        if( strcasecmp( toString( getawayId ), name ) == 0 ) {
            return get( getawayId );
        }
    }

    return nullptr;
}

IGateway* getByDevicePrefix( char prefix ) {
    for( auto getawayId = gateways::GatewayID::First; getawayId < gateways::GatewayID::Undefined; ++getawayId ) {
        if( toDevicePrefix( getawayId ) == prefix ) {
            return get( getawayId );
        }
    }

    return nullptr;
}

void enumerateGateways( std::function<bool( GatewayID, IGateway* )> clbk ) {
    for( auto getawayId = gateways::GatewayID::First; getawayId < gateways::GatewayID::Undefined; ++getawayId ) {
        auto gateway = gateways::get( getawayId );
        if( !clbk( getawayId, gateway ) ) {
            break;
        }
    }
}

}  // namespace gateways
