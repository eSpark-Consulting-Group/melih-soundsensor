#include "hub_core/GeoSettings.h"
#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"
#include "StorageCommonSection.h"
#include "values/FloatValue.h"
#include "IValueCast.h"

#include <vector>
#include <string>
#include <mutex>

#include "DebugUtils.h"

namespace core {
namespace geo_settings {

namespace {
// New York coordinates
constexpr float DefaultLatitude = 39.833851;    // 46.0f;
constexpr float DefaultLongitude = -74.871826;  // 30.0f;

const char HUB_LATITUDE_OLD_FORMAT[] = "HUB_LATITUDE";
const char HUB_LONGITUDE_OLD_FORMAT[] = "HUB_LONGITUDE";

float g_latitude{ 0.0 };
float g_longitude{ 0.0 };
std::mutex g_mutex;
}  // namespace

bool initialize() {
    std::lock_guard<std::mutex> lockGuard( g_mutex );

    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        return false;
    }

    {  // Try recovery coordinates from previous string format
        // This code block should be removed in the future
        std::vector<char> buffLat;
        std::vector<char> buffLong;
        if( store->getBlob( HUB_LATITUDE_OLD_FORMAT, buffLat ) &&
            store->getBlob( HUB_LONGITUDE_OLD_FORMAT, buffLong ) ) {
            buffLat.emplace_back( '\0' );
            buffLong.emplace_back( '\0' );
            g_latitude = std::atof( buffLat.data() );
            g_longitude = std::atof( buffLong.data() );

            common::FloatValue latitudeIValue{ g_latitude }, longitudeIValue{ g_longitude };
            if( store->set( storage::section::common::HUB_LATITUDE, latitudeIValue ) &&
                store->set( storage::section::common::HUB_LONGITUDE, longitudeIValue ) ) {
                store->remove( HUB_LATITUDE_OLD_FORMAT );
                store->remove( HUB_LONGITUDE_OLD_FORMAT );
            }

            return true;
        }
    }

    auto latitudeIValue = store->getIValue( storage::section::common::HUB_LATITUDE );
    auto longitudeIValue = store->getIValue( storage::section::common::HUB_LONGITUDE );
    if( latitudeIValue && longitudeIValue ) {
        g_latitude = common::iValueCast<float>( latitudeIValue );
        g_longitude = common::iValueCast<float>( longitudeIValue );
        return true;
    }

    FUNC_LOGW( "Set default coordinates" );
    g_latitude = DefaultLatitude;
    g_longitude = DefaultLongitude;
    return true;
}

bool saveGeoCoordinates( const float latitude, const float longitude ) {
    std::lock_guard<std::mutex> lockGuard( g_mutex );
    g_latitude = latitude;
    g_longitude = longitude;

    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    common::FloatValue latitudeIValue{ latitude }, longitudeIValue{ longitude };
    return store && store->set( storage::section::common::HUB_LATITUDE, latitudeIValue ) &&
           store->set( storage::section::common::HUB_LONGITUDE, longitudeIValue );
}

std::tuple<float, float> getCoordinates() {
    std::lock_guard<std::mutex> lockGuard( g_mutex );
    return std::make_tuple( g_latitude, g_longitude );
}

bool isDefaultCoords() {
    std::lock_guard<std::mutex> lockGuard( g_mutex );
    return g_latitude == DefaultLatitude && g_longitude == DefaultLongitude;
}

}  // namespace geo_settings
}  // namespace core
