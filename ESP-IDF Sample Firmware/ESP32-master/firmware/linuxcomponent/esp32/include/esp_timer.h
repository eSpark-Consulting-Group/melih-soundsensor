#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get time in microseconds since boot
 * @return number of microseconds since esp_timer_init was called (this normally
 *          happens early during application startup).
 */
int64_t esp_timer_get_time();

#ifdef __cplusplus
}
#endif
