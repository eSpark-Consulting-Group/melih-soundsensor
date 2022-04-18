#include <cstring>

#include "EspLogger.h"
#include "EspLogTags.h"

#define LOGW( ... ) ESP_LOGW(::esp_log_tags::ESP_LOGGER, __VA_ARGS__ )

namespace core {

static const char* ESP_LOG_LEVEL_STR[] = { "ESP_LOG_NONE", "ESP_LOG_ERROR", "ESP_LOG_WARN",
                                           "ESP_LOG_INFO", "ESP_LOG_DEBUG", "ESP_LOG_VERBOSE" };

EspLogger::EspLogger(::storage::KeyValueStorageFactory::StoragePtr storage )
: m_storage( std::move( storage ) ) {
    ::esp_log_level_t defaultLevel;
    if( !parseLogLevel( DEFAULT_ESP_LOG_LEVEL, defaultLevel ) ) {
        LOGW( "Cannot parse DEFAULT_ESP_LOG_LEVEL value" );
        defaultLevel = ::esp_log_level_t::ESP_LOG_INFO;
    }
    LOGW( "Default log level is set to %s (%d)", ESP_LOG_LEVEL_STR[defaultLevel], defaultLevel );
    setDefaultLogLevel(::esp_log_tags::CORE, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::LUA, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::LUA_BINDING, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::LUA_TIMER, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::ZWAVE, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::ZNET_DAILY_REPEATING_SCHEDULE, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::ZNET_WEEK_DAY_SCHEDULE, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::ZNET_YEAR_DAY_SCHEDULE, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::ZNET_SOUND_SWITCH_CONFIG, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::ZNET_SOUND_SWITCH_TONE, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::RAM_BUFFER_STREAM, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::BINARY_FILE_STORAGE, defaultLevel );
    setDefaultLogLevel(::esp_log_tags::VALUE_TO_LUA, defaultLevel );
    reconfigureLogLevels();
}

void EspLogger::setLogLevel( const char* tag, ::esp_log_level_t level ) {
    m_storage->setInt32( tag, level );
    if( 0 == ::strcmp( tag, ::esp_log_tags::GLOBAL_WILDCARD ) ) {
        LOGW( "Initiated update of global log level." );
        reconfigureLogLevels();
    }
    else {
        ::esp_log_level_set( tag, level );
        LOGW( "Log level of %s is set to %s (%d)", tag, ESP_LOG_LEVEL_STR[level], level );
    }
}

void EspLogger::setDefaultLogLevel( const char* tag, ::esp_log_level_t level ) {
    if( !m_storage->exists( tag ) ) {
        m_storage->setInt32( tag, level );
    }
}

void EspLogger::reconfigureLogLevels() {
    // Since the global wildcard flushes all current rules, gotta set it in the very first order.
    ::esp_log_level_t globalLogLevelParsed = ESP_LOG_VERBOSE;
    if( m_storage->exists(::esp_log_tags::GLOBAL_WILDCARD ) ) {
        std::int32_t globalLogLevelValue;
        if( m_storage->getInt32(::esp_log_tags::GLOBAL_WILDCARD, globalLogLevelValue ) &&
            parseLogLevel( globalLogLevelValue, globalLogLevelParsed ) ) {
            ::esp_log_level_set(::esp_log_tags::GLOBAL_WILDCARD, globalLogLevelParsed );
        }
    }

    // Override global log level for this module to always see log level updates.
    ::esp_log_level_set(::esp_log_tags::ESP_LOGGER, ::esp_log_level_t::ESP_LOG_VERBOSE );

    LOGW( "Global log level is set to %s (%d)", ESP_LOG_LEVEL_STR[globalLogLevelParsed], globalLogLevelParsed );

    // Set the rest of rules.
    auto iter = m_storage->cbegin();
    while( iter != m_storage->cend() ) {
        auto value = iter.value();
        ::esp_log_level_t level;
        if( *iter != ::esp_log_tags::GLOBAL_WILDCARD && value.is_type<std::int32_t>() &&
            parseLogLevel( value.get<std::int32_t>(), level ) ) {
            ::esp_log_level_set( ( *iter ).c_str(), level );
            LOGW( "Log level of %s is set to %s (%d)", ( *iter ).c_str(), ESP_LOG_LEVEL_STR[level], level );
        }
        ++iter;
    }
}

bool EspLogger::parseLogLevel( int numericValue, esp_log_level_t& level ) {
    switch( numericValue ) {
    case ::esp_log_level_t::ESP_LOG_NONE:
        level = ::esp_log_level_t::ESP_LOG_NONE;
        return true;
    case ::esp_log_level_t::ESP_LOG_ERROR:
        level = ::esp_log_level_t::ESP_LOG_ERROR;
        return true;
    case ::esp_log_level_t::ESP_LOG_WARN:
        level = ::esp_log_level_t::ESP_LOG_WARN;
        return true;
    case ::esp_log_level_t::ESP_LOG_INFO:
        level = ::esp_log_level_t::ESP_LOG_INFO;
        return true;
    case ::esp_log_level_t::ESP_LOG_DEBUG:
        level = ::esp_log_level_t::ESP_LOG_DEBUG;
        return true;
    case ::esp_log_level_t::ESP_LOG_VERBOSE:
        level = ::esp_log_level_t::ESP_LOG_VERBOSE;
        return true;
    default:
        return false;
    }
}

}  // namespace core
