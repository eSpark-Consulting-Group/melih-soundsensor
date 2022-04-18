#include "time_service/Timezone.h"

#include "BinaryFileStorageFactory.h"
#include "StorageSection.h"
#include "StorageCommonSection.h"

#include "Bus.h"
#include "BusCommandTimezoneWasUpdated.h"

#include <vector>
#include <mutex>

#include "DebugUtils.h"

namespace time_service {

namespace {

constexpr char DefaultTimeZoneId[] = "America/New_York";
constexpr char DefaultTzEnvVar[] = "EST5EDT,M3.2.0,M11.1.0";  // It's for America/New_York

std::string g_currLocation;
std::string g_currTzEnvVar;
std::mutex g_mutex;

bool setTimeZone( const std::string& tzValue ) {
    if( ( tzValue.empty() && 0 != unsetenv( "TZ" ) ) || ( 0 != setenv( "TZ", tzValue.c_str(), 1 ) ) ) {
        return false;
    }

    tzset();
    return true;
}

}  // namespace

bool initTimezoneData() {
    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        return false;
    }

    {
        storage::KeyValueStorage::BlobType blob;
        if( store->getBlob( storage::section::common::HUB_TIMEZONEID, blob ) && !blob.empty() ) {
            g_currLocation.reserve( blob.size() );
            g_currLocation.insert( g_currLocation.begin(), blob.begin(), blob.end() );
        }
    }

    {
        storage::KeyValueStorage::BlobType blob;
        if( store->getBlob( storage::section::common::HUB_TZENVVAR, blob ) && !blob.empty() ) {
            g_currTzEnvVar.reserve( blob.size() );
            g_currTzEnvVar.insert( g_currTzEnvVar.begin(), blob.begin(), blob.end() );
        }
    }

    const auto tzVar = getTzEnvVariable();
    const auto tzId = getTimeZoneId();
    if( setTimeZone( tzVar ) ) {
        FUNC_LOGI( "Init timezone=%s descr=%s", tzId.c_str(), tzVar.c_str() );
        return true;
    }
    else {
        FUNC_LOGE( "Can't init timezone" );
        return false;
    }
}

bool setTzEnvVar( const std::string tzLocation, const std::string tzVar ) {
    if( tzLocation.empty() || tzVar.empty() ) {
        return false;
    }

    std::lock_guard<decltype( g_mutex )> lock{ g_mutex };

    if( g_currLocation == tzLocation && g_currTzEnvVar == tzVar ) {
        return true;
    }

    if( !setTimeZone( tzVar ) ) {
        return false;
    }
    g_currLocation = tzLocation;
    g_currTzEnvVar = tzVar;
    bus::sendFromNowhere<bus::command::TimezoneWasUpdated>( "znet" );

    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::COMMON );
    if( !store ) {
        return false;
    }

    {
        storage::KeyValueStorage::BlobType blob;
        blob.reserve( tzLocation.length() );
        blob.insert( blob.begin(), tzLocation.begin(), tzLocation.end() );
        if( !store->setBlob( storage::section::common::HUB_TIMEZONEID, blob ) ) {
            return false;
        }
    }

    {
        storage::KeyValueStorage::BlobType blob;
        blob.reserve( tzVar.length() );
        blob.insert( blob.begin(), tzVar.begin(), tzVar.end() );
        if( !store->setBlob( storage::section::common::HUB_TZENVVAR, blob ) ) {
            return false;
        }
    }

    return true;
}

const std::string getTimeZoneId() {
    std::lock_guard<decltype( g_mutex )> lock{ g_mutex };

    return g_currTzEnvVar.empty() ? DefaultTimeZoneId : g_currLocation;
}

bool isDefaultTimezone() {
    std::lock_guard<decltype( g_mutex )> lock{ g_mutex };

    return g_currTzEnvVar.empty();
}

const std::string getTzEnvVariable() {
    std::lock_guard<decltype( g_mutex )> lock{ g_mutex };

    return g_currTzEnvVar.empty() ? DefaultTzEnvVar : g_currTzEnvVar;
}

}  // namespace time_service
