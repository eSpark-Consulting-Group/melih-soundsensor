#pragma once

#include <tuple>

namespace core {
namespace geo_settings {

bool initialize();

bool saveGeoCoordinates( const float latitude, const float longitude );

std::tuple<float, float> getCoordinates();  // return Latitude, Longitude

bool isDefaultCoords();

}  // namespace geo_settings
}  // namespace core
