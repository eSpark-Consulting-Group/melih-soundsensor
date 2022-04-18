#pragma once

#include <esp_log.h>

#include "KeyValueStorageFactory.h"

namespace core {

/**
 * Configures log level of a native esp logger and stores its configuration to a key-value storage.
 * Provides interface to safely modify log levels at runtime.
 */
class EspLogger {
public:
    /**
     * Loads log levels from the storage and initializes rules.
     * Also sets the default levels for some tags defined at esp_log_tags namespace.
     * @param storage Key-value storage where log level rules are stored.
     */
    explicit EspLogger(::storage::KeyValueStorageFactory::StoragePtr storage );

    /**
     * Sets max log level for log records with a given tag.
     * Each change causes update of the rule storage.
     * All updates take effect right away.
     * @param tag Tag that the level is applied to. Known tags are listed at esp_log_tags namespace.
     * @param level The most verbose level to be logged.
     */
    void setLogLevel( const char* tag, ::esp_log_level_t level );

private:
    void setDefaultLogLevel( const char* tag, ::esp_log_level_t level );
    void reconfigureLogLevels();

    static bool parseLogLevel( int numericValue, ::esp_log_level_t& level );

private:
    ::storage::KeyValueStorageFactory::StoragePtr m_storage;
};

}  // namespace core
