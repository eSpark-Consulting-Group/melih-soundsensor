#include <sdkconfig.h>
#if CONFIG_REMOTE_LOGGING

#include <RemoteLogger.h>
#include <cstdio>
#include <Ezlo.h>
#include <JsonStreamGenerator.h>
#include <CommonResources.h>
#include <esp_log.h>
#include <cstring>
#include <BusCommandRemoteLog.h>

#define LOG_START( letter ) LOG_COLOR_##letter #letter " (%d) %s: "
#define LOG_END() LOG_RESET_COLOR "\n"

namespace core {

namespace {

const char* STRING_ERROR = "ERROR";
const char* STRING_WARNING = "WARNING";
const char* STRING_DEBUG = "DEBUG";
const char* STRING_TRACE = "TRACE";
const char* STRING_INFO = "INFO";
const char* STRING_ALL = "ALL";
}  // namespace

void Logger::write( const Level level, const char* tag, const char* format, ... ) {
    std::array<char, MessageBufferSize> buffer;

    va_list args;
    va_start( args, format );
    if( 0 > vsnprintf( buffer.data(), buffer.size(), format, args ) ) {
        va_end( args );
        return;
    }
    va_end( args );

    ESP_LOG_LEVEL( getEspLogLevel( level ), tag, "%s", buffer.data() );

    if( m_isAllowed && level >= m_logLevel ) {
        auto command = std::make_unique<bus::command::RemoteLog>( tag, esp_log_timestamp(), level, buffer.data() );
        send( "nma", std::move( command ) );
    }
}

const char* Logger::LevelToText( Level level ) {
    switch( level ) {
    case ERROR:
        return STRING_ERROR;
    case WARNING:
        return STRING_WARNING;
    case DEBUG:
        return STRING_DEBUG;
    case TRACE:
        return STRING_TRACE;
    default:
        return STRING_INFO;
    }
}

void Logger::setLogLevel( const char* level ) {
    if( 0 == strcmp( level, STRING_ERROR ) ) {
        setLogLevel( ERROR );
    }
    else if( 0 == strcmp( level, STRING_WARNING ) ) {
        setLogLevel( WARNING );
    }
    else if( 0 == strcmp( level, STRING_INFO ) ) {
        setLogLevel( INFO );
    }
    else if( 0 == strcmp( level, STRING_DEBUG ) ) {
        setLogLevel( DEBUG );
    }
    else if( 0 == strcmp( level, STRING_TRACE ) ) {
        setLogLevel( TRACE );
    }
    else if( 0 == strcmp( level, STRING_ALL ) ) {
        setLogLevel( ALL );
    }
    else {
        setLogLevel( NONE );
    }
}

Logger::Level Logger::getLogLevel() {
    return m_logLevel;
}

::esp_log_level_t Logger::getEspLogLevel( Logger::Level level ) {
    switch( level ) {
    case ERROR:
        return ::esp_log_level_t::ESP_LOG_ERROR;
    case WARNING:
        return ::esp_log_level_t::ESP_LOG_WARN;
    case DEBUG:
        return ::esp_log_level_t::ESP_LOG_DEBUG;
    case TRACE:
        return ::esp_log_level_t::ESP_LOG_VERBOSE;
    default:
        return ::esp_log_level_t::ESP_LOG_INFO;
    }
}

bool Logger::initializeImpl() {
    if( !core::SingletonBaseImpl<Logger>::initializeImpl() ) {
        return false;
    }

    if( !bus::Bus::get().join( cloud_client::responces::BUS_NAME, *this ) ) {
        return false;
    }

    esp_log_level_set( "phy_init", ESP_LOG_INFO );
    esp_log_level_set( "wifi", ESP_LOG_INFO );
    esp_log_level_set( "phy_init", ESP_LOG_INFO );

    return true;
}

void Logger::visit( const bus::BusAddr from,
                    const bus::BusAddr to,
                    const cloud_client::responces::ConnectedToCloud& command ) {
    setAllowed( true );
}

void Logger::visit( const bus::BusAddr from,
                    const bus::BusAddr to,
                    const cloud_client::responces::DisconnectedFromCloud& command ) {
    setAllowed( false );
}

}  // namespace core

#endif  // CONFIG_REMOTE_LOGGING
