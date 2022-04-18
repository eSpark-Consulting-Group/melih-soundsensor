#pragma once

#include <string>
#include <tuple>

namespace common {
class JsonStreamGenerator;
}  // namespace common

namespace gason {
class JsonValue;
}  // namespace gason

namespace scenes {

bool updateHubScenesTriggerDevicesList( const bool isRebuild = false );

std::tuple<int, std::string> onHubScenesBlocksList( const gason::JsonValue& json, common::JsonStreamGenerator& gen );

std::tuple<int, std::string> onHubScenesTriggerDevicesList( common::JsonStreamGenerator& gen );

}  // namespace scenes
