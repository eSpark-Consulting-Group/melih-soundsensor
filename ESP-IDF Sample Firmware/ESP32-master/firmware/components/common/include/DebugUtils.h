
#pragma once

#include <stdint.h>
#include <stdio.h>

#include <sdkconfig.h>
#include <RemoteLogger.h>
#include <chrono>

#include <esp_log.h>

namespace common {
namespace debug {

class DurationMeasurer {
public:
    template <typename Duration = std::chrono::milliseconds>
    Duration get() const {
        return std::chrono::duration_cast<Duration>( std::chrono::steady_clock::now() - m_timePoint );
    }

private:
    std::chrono::steady_clock::time_point m_timePoint{ std::chrono::steady_clock::now() };
};

void mlogUint32( const char* file, const char* func, int line, const char* name, uint32_t value );

void mlogUint8( const char* file, const char* func, int line, const char* name, uint8_t value );

void mlogUint8Array( const char* file, const char* func, int line, const char* name, uint8_t* value, uint32_t count );

}  // namespace debug
}  // namespace common

#if defined( LOG_XX ) || defined( LOG_XX_ARR ) || defined( FUNC_LOGE ) || defined( FUNC_LOGW ) || \
    defined( FUNC_LOGI ) || defined( FUNC_LOGD )

#error the redefinition of LOG_XX macro detected

#else

#define LOG_XX( func, value )                                                         \
    do {                                                                              \
        ::common::debug::func( __FILE__, __FUNCTION__, __LINE__, #value, ( value ) ); \
    } while( 0 );

#define LOG_XX_ARR( func, value, length )                                                         \
    do {                                                                                          \
        ::common::debug::func( __FILE__, __FUNCTION__, __LINE__, #value, ( value ), ( length ) ); \
    } while( 0 );

#define LOG_U32( value ) LOG_XX( mlogUint32, value )

#define LOG_U8( value ) LOG_XX( mlogUint8, value )

#define LOG_U8_ARR( value, length ) LOG_XX_ARR( mlogUint8Array, value, length )

#ifdef CONFIG_REMOTE_LOGGING
#define REMOTE_LOGE( tag, format, ... )                                       \
    do {                                                                      \
        auto logger = core::Logger::get();                                    \
        if( logger ) {                                                        \
            logger->write( core::Logger::ERROR, tag, format, ##__VA_ARGS__ ); \
        }                                                                     \
    } while( false )

#define REMOTE_LOGW( tag, format, ... )                                         \
    do {                                                                        \
        auto logger = core::Logger::get();                                      \
        if( logger ) {                                                          \
            logger->write( core::Logger::WARNING, tag, format, ##__VA_ARGS__ ); \
        }                                                                       \
    } while( false )

#define REMOTE_LOGI( tag, format, ... )                                      \
    do {                                                                     \
        auto logger = core::Logger::get();                                   \
        if( logger ) {                                                       \
            logger->write( core::Logger::INFO, tag, format, ##__VA_ARGS__ ); \
        }                                                                    \
    } while( false )

#define REMOTE_LOGD( tag, format, ... )                                       \
    do {                                                                      \
        auto logger = core::Logger::get();                                    \
        if( logger ) {                                                        \
            logger->write( core::Logger::DEBUG, tag, format, ##__VA_ARGS__ ); \
        }                                                                     \
    } while( false )

#define REMOTE_LOGV( tag, format, ... )                                       \
    do {                                                                      \
        auto logger = core::Logger::get();                                    \
        if( logger ) {                                                        \
            logger->write( core::Logger::TRACE, tag, format, ##__VA_ARGS__ ); \
        }                                                                     \
    } while( false )

#else  // CONFIG_REMOTE_LOGGING

#define REMOTE_LOGE( tag, format, ... )
#define REMOTE_LOGW( tag, format, ... )
#define REMOTE_LOGI( tag, format, ... )
#define REMOTE_LOGD( tag, format, ... )
#define REMOTE_LOGV( tag, format, ... )

#endif  // CONFIG_REMOTE_LOGGING

#define FUNC_LOGE( ... ) ESP_LOGE( __FUNCTION__, __VA_ARGS__ )
#define FUNC_LOGW( ... ) ESP_LOGW( __FUNCTION__, __VA_ARGS__ )
#define FUNC_LOGI( ... ) ESP_LOGI( __FUNCTION__, __VA_ARGS__ )
#define FUNC_LOGD( ... ) ESP_LOGD( __FUNCTION__, __VA_ARGS__ )
#define FUNC_LOGV( ... ) ESP_LOGV( __FUNCTION__, __VA_ARGS__ )

#endif
