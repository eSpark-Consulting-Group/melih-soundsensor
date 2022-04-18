#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <assert.h>

int64_t esp_timer_get_time() {
    struct timespec timespec = {};
    int retCode = clock_settime( CLOCK_MONOTONIC, &timespec );
    assert( retCode == 0 );
    return timespec.tv_sec * 1e6 + timespec.tv_nsec;
}
