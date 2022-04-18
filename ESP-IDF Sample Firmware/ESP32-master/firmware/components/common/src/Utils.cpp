#include <Utils.h>

#include <values/BoolValue.h>
#include <values/FloatValue.h>
#include <values/IntValue.h>
#include <values/ArrayValue.h>
#include <values/TableValue.h>
#include <values/StringValue.h>
#include <values/ScalebleValue.h>
#include <IValueVisitor.h>
#include <IValueCast.h>

#include <DeviceInfo.h>
#include <ObjectSerializer.h>
#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <CommonResources.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <gason.hpp>

#include <thread>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <DebugUtils.h>

extern "C" {

uint8_t temprature_sens_read();
}

namespace common {
namespace {
double convertBytesToKilobytes( int32_t bytes ) {
    return bytes != 0 ? bytes / 1024.0 : 0.0;
}

const char AverageTag[] = "AVERAGE";

}  // namespace

std::string hexToString( const unsigned char* data, size_t size, bool upperCase ) {
    std::ostringstream result;

    result << std::hex << std::setfill( '0' ) << ( upperCase ? std::uppercase : std::nouppercase );
    for( size_t i = 0; i < size; ++i ) {
        result << std::setw( 2 ) << (int)data[i];
    }
    return result.str();
}

static int count_leap_year( int y ) {
    y -= 1;
    return y / 4 - y / 100 + y / 400;
}

time_t nghttp2_timegm( struct tm* tm ) {
    int days;
    int num_leap_year;
    int64_t t;
    if( tm->tm_mon > 11 ) {
        return -1;
    }
    num_leap_year = count_leap_year( tm->tm_year + 1900 ) - count_leap_year( 1970 );
    days = ( tm->tm_year - 70 ) * 365 + num_leap_year + tm->tm_yday;
    t = ( (int64_t)days * 24 + tm->tm_hour ) * 3600 + tm->tm_min * 60 + tm->tm_sec;

#if SIZEOF_TIME_T == 4
    if( t < INT32_MIN || t > INT32_MAX ) {
        return -1;
    }
#endif /* SIZEOF_TIME_T == 4 */

    return (time_t)t;
}

void printSystemAverage( bool sendRemote ) {
    multi_heap_info_t info = {};
    heap_caps_get_info( &info, MALLOC_CAP_8BIT );

#ifdef CONFIG_SPIRAM
    multi_heap_info_t info_internal = {};
    heap_caps_get_info( &info_internal, MALLOC_CAP_INTERNAL );
    static auto internalFreeBytes = info_internal.total_free_bytes;
    static auto internalAllocatedBytes = info_internal.total_allocated_bytes;
    static auto internalMinimumFreeBytes = info_internal.minimum_free_bytes;
    static auto internalLargestFreeBlock = info_internal.largest_free_block;
#endif
    auto currentTempFarenheit = temprature_sens_read();

    static auto totalFreeBytes = info.total_free_bytes;
    static auto totalAllocatedBytes = info.total_allocated_bytes;
    static auto minimumFreeBytes = info.minimum_free_bytes;
    static auto largestFreeBlock = info.largest_free_block;
    static auto lastTempFarenheit = currentTempFarenheit;

    auto tempDiff = currentTempFarenheit - lastTempFarenheit;

    //    if( ( info.total_free_bytes != totalFreeBytes ) || ( info.total_allocated_bytes != totalAllocatedBytes ) ||
    //        ( info.minimum_free_bytes != minimumFreeBytes ) || ( info.largest_free_block != largestFreeBlock ) ||
    //        ( fabs( tempDiff ) >= 32 ) )
    {
        if( sendRemote ) {
#ifdef CONFIG_SPIRAM

            REMOTE_LOGD( AverageTag,
                         "= [  Free(i)   | Allocated(i) | Minimum(i) | Largest Block(i) | Stack(i)  |   °C   ] =" );

            REMOTE_LOGD( AverageTag,
                         "= [ %-7.2f(%-7.2f) | %-9.2f(%-7.2f) | %-7.2f(%-7.2f) | %-13.2f(%-7.2f) | %-6.2f | %-6.2f ] =",
                         convertBytesToKilobytes( info.total_free_bytes ),
                         convertBytesToKilobytes( info_internal.total_free_bytes ),
                         convertBytesToKilobytes( info.total_allocated_bytes ),
                         convertBytesToKilobytes( info_internal.total_allocated_bytes ),
                         convertBytesToKilobytes( info.minimum_free_bytes ),
                         convertBytesToKilobytes( info_internal.minimum_free_bytes ),
                         convertBytesToKilobytes( info.largest_free_block ),
                         convertBytesToKilobytes( info_internal.largest_free_block ),
                         convertBytesToKilobytes( uxTaskGetStackHighWaterMark( NULL ) ),

                         ( currentTempFarenheit - 32 ) / 1.8 );

            REMOTE_LOGD( AverageTag,
                         "= [ %+-7.2f(%+-7.2f) | %+-9.2f(%+-9.2f) | %+-7.2f(%+-7.2f) | %+-13.2f(%+-13.2f) |   -    | "
                         "%+-6.2f ] =",
                         convertBytesToKilobytes( info.total_free_bytes - totalFreeBytes ),
                         convertBytesToKilobytes( info_internal.total_free_bytes - internalFreeBytes ),
                         convertBytesToKilobytes( info.total_allocated_bytes - totalAllocatedBytes ),
                         convertBytesToKilobytes( info_internal.total_allocated_bytes - internalAllocatedBytes ),
                         convertBytesToKilobytes( info.minimum_free_bytes - minimumFreeBytes ),
                         convertBytesToKilobytes( info_internal.minimum_free_bytes - internalMinimumFreeBytes ),
                         convertBytesToKilobytes( info.largest_free_block - largestFreeBlock ),
                         convertBytesToKilobytes( info_internal.largest_free_block - internalLargestFreeBlock ),
                         ( ( ( currentTempFarenheit - 32 ) / 1.8 ) - ( ( lastTempFarenheit - 32 ) / 1.8 ) ) );
#else
            REMOTE_LOGD( AverageTag, "= [  Free   | Allocated | Minimum | Largest Block | Stack  |   °C   ] =" );

            REMOTE_LOGD( AverageTag,
                         "= [ %-7.2f | %-9.2f | %-7.2f | %-13.2f | %-6.2f | %-6.2f ] =",
                         convertBytesToKilobytes( info.total_free_bytes ),
                         convertBytesToKilobytes( info.total_allocated_bytes ),
                         convertBytesToKilobytes( info.minimum_free_bytes ),
                         convertBytesToKilobytes( info.largest_free_block ),
                         convertBytesToKilobytes( uxTaskGetStackHighWaterMark( NULL ) ),

                         ( currentTempFarenheit - 32 ) / 1.8 );

            REMOTE_LOGD( AverageTag,
                         "= [ %+-7.2f | %+-9.2f | %+-7.2f | %+-13.2f |   -    | %+-6.2f ] =",
                         convertBytesToKilobytes( info.total_free_bytes - totalFreeBytes ),
                         convertBytesToKilobytes( info.total_allocated_bytes - totalAllocatedBytes ),
                         convertBytesToKilobytes( info.minimum_free_bytes - minimumFreeBytes ),
                         convertBytesToKilobytes( info.largest_free_block - largestFreeBlock ),
                         ( ( ( currentTempFarenheit - 32 ) / 1.8 ) - ( ( lastTempFarenheit - 32 ) / 1.8 ) ) );
#endif
        }
        else {
#ifdef CONFIG_SPIRAM

            ESP_LOGI( AverageTag,
                      "= [  Free(i)   | Allocated(i) | Minimum(i) | Largest Block(i) | Stack(  |   °C   ] =" );

            ESP_LOGI( AverageTag,
                      "= [ %-7.2f(%-7.2f) | %-9.2f(%-7.2f) | %-7.2f(%-7.2f) | %-13.2f(%-7.2f) | %-6.2f | %-6.2f ] =",
                      convertBytesToKilobytes( info.total_free_bytes ),
                      convertBytesToKilobytes( info_internal.total_free_bytes ),
                      convertBytesToKilobytes( info.total_allocated_bytes ),
                      convertBytesToKilobytes( info_internal.total_allocated_bytes ),
                      convertBytesToKilobytes( info.minimum_free_bytes ),
                      convertBytesToKilobytes( info_internal.minimum_free_bytes ),
                      convertBytesToKilobytes( info.largest_free_block ),
                      convertBytesToKilobytes( info_internal.largest_free_block ),
                      convertBytesToKilobytes( uxTaskGetStackHighWaterMark( NULL ) ),

                      ( currentTempFarenheit - 32 ) / 1.8 );

            ESP_LOGI( AverageTag,
                      "= [ %+-7.2f(%+-7.2f) | %+-9.2f(%+-9.2f) | %+-7.2f(%+-7.2f) | %+-13.2f(%+-13.2f) |   -    | "
                      "%+-6.2f ] =",
                      convertBytesToKilobytes( info.total_free_bytes - totalFreeBytes ),
                      convertBytesToKilobytes( info_internal.total_free_bytes - internalFreeBytes ),
                      convertBytesToKilobytes( info.total_allocated_bytes - totalAllocatedBytes ),
                      convertBytesToKilobytes( info_internal.total_allocated_bytes - internalAllocatedBytes ),
                      convertBytesToKilobytes( info.minimum_free_bytes - minimumFreeBytes ),
                      convertBytesToKilobytes( info_internal.minimum_free_bytes - internalMinimumFreeBytes ),
                      convertBytesToKilobytes( info.largest_free_block - largestFreeBlock ),
                      convertBytesToKilobytes( info_internal.largest_free_block - internalLargestFreeBlock ),
                      ( ( ( currentTempFarenheit - 32 ) / 1.8 ) - ( ( lastTempFarenheit - 32 ) / 1.8 ) ) );
#else
            ESP_LOGI( AverageTag, "= [  Free   | Allocated | Minimum | Largest Block | Stack  |   °C   ] =" );

            ESP_LOGI( AverageTag,
                      "= [ %-7.2f | %-9.2f | %-7.2f | %-13.2f | %-6.2f | %-6.2f ] =",
                      convertBytesToKilobytes( info.total_free_bytes ),
                      convertBytesToKilobytes( info.total_allocated_bytes ),
                      convertBytesToKilobytes( info.minimum_free_bytes ),
                      convertBytesToKilobytes( info.largest_free_block ),
                      convertBytesToKilobytes( uxTaskGetStackHighWaterMark( NULL ) ),

                      ( currentTempFarenheit - 32 ) / 1.8 );

            ESP_LOGI( AverageTag,
                      "= [ %+-7.2f | %+-9.2f | %+-7.2f | %+-13.2f |   -    | %+-6.2f ] =",
                      convertBytesToKilobytes( info.total_free_bytes - totalFreeBytes ),
                      convertBytesToKilobytes( info.total_allocated_bytes - totalAllocatedBytes ),
                      convertBytesToKilobytes( info.minimum_free_bytes - minimumFreeBytes ),
                      convertBytesToKilobytes( info.largest_free_block - largestFreeBlock ),
                      ( ( ( currentTempFarenheit - 32 ) / 1.8 ) - ( ( lastTempFarenheit - 32 ) / 1.8 ) ) );
#endif
        }

        totalFreeBytes = info.total_free_bytes;
        totalAllocatedBytes = info.total_allocated_bytes;
        minimumFreeBytes = info.minimum_free_bytes;
        largestFreeBlock = info.largest_free_block;
        lastTempFarenheit = currentTempFarenheit;
#ifdef CONFIG_SPIRAM
        static auto internalFreeBytes = info_internal.total_free_bytes;
        static auto internalAllocatedBytes = info_internal.total_allocated_bytes;
        static auto internalMinimumFreeBytes = info_internal.minimum_free_bytes;
        static auto internalLargestFreeBlock = info_internal.largest_free_block;
#endif
    }
#ifdef EXTENDED_MEMORY_DEBUG
    heap_caps_print_heap_info( MALLOC_CAP_8BIT );
#endif
}

void waitWhileFunctionIsTrueOrTimeout( std::function<bool()> f,
                                       std::chrono::milliseconds waitingDelayMs,
                                       unsigned int maxWaitingTries ) {
    unsigned int tries = 0;
    while( tries < maxWaitingTries && f() ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( waitingDelayMs ) );
        tries++;
    }
}

uint32_t getTimeSinceBootInSec() {
    return esp_timer_get_time() / 1000000LL;
}

bool isMoreTimeSinceTimestampThanInSec( uint32_t lastTimestamp, uint32_t timeBorder ) {
    return ( ( getTimeSinceBootInSec() - lastTimestamp ) >= timeBorder );
}

bool stringEndsWith( const char* source, const char* suffix ) {
    if( !source || !suffix ) {
        return false;
    }
    size_t source_length = std::strlen( source );
    size_t suffix_length = std::strlen( suffix );
    if( source_length < suffix_length ) {
        return false;
    }
    return 0 == std::strncmp( &source[source_length - suffix_length], suffix, suffix_length );
}

bool stringStartsWith( const char* source, const char* prefix ) {
    if( !source || !prefix ) {
        return false;
    }
    size_t source_length = std::strlen( source );
    size_t prefix_length = std::strlen( prefix );
    if( source_length < prefix_length ) {
        return false;
    }
    return 0 == std::strncmp( source, prefix, prefix_length );
}

int uuidBinToStr( const char* bin, char* str, size_t max_len ) {
    return snprintf( str,
                     max_len,
                     "%02x%02x%02x%02x-%02x%02x-%02x%02x-"
                     "%02x%02x-%02x%02x%02x%02x%02x%02x",
                     bin[0],
                     bin[1],
                     bin[2],
                     bin[3],
                     bin[4],
                     bin[5],
                     bin[6],
                     bin[7],
                     bin[8],
                     bin[9],
                     bin[10],
                     bin[11],
                     bin[12],
                     bin[13],
                     bin[14],
                     bin[15] );
}

IValue::UPtr makeIval( const gason::JsonValue& value ) {
    switch( value.getTag() ) {
    case gason::JSON_NUMBER: {
        bool is_success = false;
        float fltVal = value.toNumber( &is_success );
        if( is_success ) {
            if( std::floor( fltVal ) == fltVal ) {
                return std::make_unique<IntValue>( fltVal );
            }
            else {
                return std::make_unique<FloatValue>( fltVal );
            }
        }

        return nullptr;
    }

    case gason::JSON_STRING:
        return std::make_unique<StringValue>( value.toString() );

    case gason::JSON_ARRAY: {
        auto arrVal = std::make_unique<ArrayValue>();
        for( auto jsNode : value ) {
            auto val = makeIval( jsNode.value );
            arrVal->push_back( std::move( val ) );
        }
        return arrVal;
    }

    case gason::JSON_OBJECT: {
        auto tblVal = std::make_unique<TableValue>();
        for( auto jsNode : value ) {
            auto val = makeIval( jsNode.value );
            tblVal->insert( jsNode.key, std::move( val ) );
        }
        return tblVal;
    }

    case gason::JSON_TRUE:
    case gason::JSON_FALSE:
        return std::make_unique<BoolValue>( value.toBool() );

    case gason::JSON_NULL:
        // fall throught
    default:
        return nullptr;
    }
}

std::tuple<IValue::UPtr, float, int, std::string> makeIval( const std::string& itemId, const float percent ) {
    using namespace common::resources;

    auto store = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ITEMS );
    if( !store ) {
        return std::make_tuple( nullptr, NAN, errors::InternalError, errors::data::EzloInternaleError );
    }

    gateways::ItemInfo itemInfo;
    {
        gateways::ObjectSerializer serializer;
        storage::KeyValueStorage::BlobType buff;
        if( !store->getBlob( itemId, buff ) || !serializer.deserialize( std::move( buff ), itemInfo ) ) {
            return std::make_tuple( nullptr, NAN, errors::InternalError, errors::data::EzloInternaleError );
        }
        itemInfo.setId( itemId.c_str() );
    }

    IValue::UPtr ival;
    int errorCode;
    std::string errorMessage;
    std::tie( ival, errorCode, errorMessage ) = itemInfo.createValue( percent );
    auto newValue = iValueCast<float>( ival );
    return std::make_tuple( std::move( ival ), newValue, errorCode, std::move( errorMessage ) );
}

bool isServiceNotification( const std::string& deviceId ) {
    auto devices = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::DEVICES );
    if( !devices ) {
        return true;
    }

    std::vector<char> buff;
    if( !devices || !devices->getBlob( deviceId, buff ) ) {
        FUNC_LOGE( "Failed to get device info for the key: %s", deviceId.c_str() );
        return true;
    }

    gateways::BaseDeviceInfo deviceInfo;
    gateways::ObjectSerializer serializer;
    if( !serializer.deserialize( deviceId, std::move( buff ), deviceInfo ) ) {
        FUNC_LOGE( "Failed to deserialize device info for key: %s", deviceId.c_str() );
        return true;
    }

    return deviceInfo.isServiceNotification();
}

namespace random {

std::random_device rand_dev;
std::mt19937 generator( rand_dev() );

}  // namespace random

}  // namespace common
