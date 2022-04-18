#pragma once

/**
 * The namespace contains string literals used to tag ESP log records.
 * Tagging allows to configure log level via hub.log.local.set method.
 */
namespace esp_log_tags {

extern const char GLOBAL_WILDCARD[];
extern const char ESP_LOGGER[];
extern const char ZWAVE[];
extern const char CORE[];
extern const char LUA[];
extern const char LUA_BINDING[];
extern const char LUA_TIMER[];
extern const char RAM_BUFFER_STREAM[];
extern const char BINARY_FILE_STORAGE[];
extern const char ZNET_DAILY_REPEATING_SCHEDULE[];
extern const char ZNET_WEEK_DAY_SCHEDULE[];
extern const char ZNET_YEAR_DAY_SCHEDULE[];
extern const char ZNET_SOUND_SWITCH_CONFIG[];
extern const char ZNET_SOUND_SWITCH_TONE[];
extern const char VALUE_TO_LUA[];

}  // namespace esp_log_tags
