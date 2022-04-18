
#pragma once
#include <GatewayID.h>

namespace gateways {

void restoreDevicesInfo();

void cleanDevicesInfo( gateways::GatewayID gatewaID = gateways::GatewayID::Undefined );

void printFilesDevicesInfo();

}  // namespace gateways
