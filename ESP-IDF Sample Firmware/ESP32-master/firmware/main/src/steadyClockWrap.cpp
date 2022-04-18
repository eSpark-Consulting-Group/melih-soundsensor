// Fix for non-monotonic std::steady_clock in esp32 gcc 5.2 toolchain

#include <chrono>
#include <esp_timer.h>

extern "C" {
std::chrono::steady_clock::time_point __wrap__ZNSt6chrono3_V212steady_clock3nowEv() {
    return std::chrono::steady_clock::time_point( std::chrono::microseconds( esp_timer_get_time() ) );
}
}
