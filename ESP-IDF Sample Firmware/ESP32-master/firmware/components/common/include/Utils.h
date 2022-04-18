#pragma once

#include "IValue.h"

#include <string>
#include <functional>
#include <cstdint>
#include <chrono>
#include <tuple>
#include <random>

namespace gason {
struct JsonValue;
}

namespace common {

std::string hexToString( const unsigned char* data, size_t size, bool upperCase = false );

time_t nghttp2_timegm( struct tm* tm );

void printSystemAverage( bool sendRemote = false );

void waitWhileFunctionIsTrueOrTimeout( std::function<bool()> f,
                                       std::chrono::milliseconds waitingDelayMs,
                                       unsigned int maxWaitingTries = 1 );

uint32_t getTimeSinceBootInSec();

bool isMoreTimeSinceTimestampThanInSec( uint32_t lastTimestamp, uint32_t timeBorder );

bool stringEndsWith( const char* source, const char* suffix );

bool stringStartsWith( const char* source, const char* prefix );

int uuidBinToStr( const char* bin, char* str, size_t max_len );

IValue::UPtr makeIval( const gason::JsonValue& value );

std::tuple<IValue::UPtr, float, int, std::string> makeIval( const std::string& itemId, const float percent );

bool isServiceNotification( const std::string& deviceId );

namespace random {

extern std::random_device rand_dev;
extern std::mt19937 generator;

}  // namespace random

}  // namespace common
