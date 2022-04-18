#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_WIFI_SERVICE_ENABLE_SNIFFER

#include "Uuid.h"
#include "wifi_service2/Common.h"

#include <chrono>
#include <map>
#include <tuple>

namespace dev_dis {

using MacAddr = network::wifi_srv2::BssidType;
using Precision = unsigned int;
using Rssi = network::wifi_srv2::Rssi;
using DDClockType = std::chrono::system_clock;
using DDTimePoint = DDClockType::time_point;

using Uuid = common::Uuid;
using DevState = std::tuple<Rssi, DDTimePoint>;
using AddrToState = std::map<MacAddr, DevState>;
using UuidToState = std::map<Uuid, DevState>;

std::string toDateString( const DDTimePoint& tp );

}  // namespace dev_dis

#endif  // CONFIG_WIFI_SERVICE_ENABLE_SNIFFER
