#include <ResetZW.h>
#include <DebugUtils.h>
#include <Defs.h>
#include "sdkconfig.h"

#include "driver/gpio.h"

namespace hard {

namespace {

constexpr auto resetZWPin{ (gpio_num_t)CONFIG_IO_RESET_ZW };
constexpr auto RST_PIN_VAL{ 0 };
constexpr auto RESET_PULSE_WIDTH_MS{ 250 };

}  // namespace

ResetZW& ResetZW::get() {
    static ResetZW resetzw;
    return resetzw;
}

bool ResetZW::initialize() {
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ( 1ULL << CONFIG_IO_RESET_ZW );

    return ESP_OK == gpio_config( &io_conf ) && ESP_OK == gpio_set_level( resetZWPin, RST_PIN_VAL );
}

void ResetZW::resetPinAssert() {
    FUNC_LOGD( "Zwave module reset pin assert" );
    gpio_set_level( resetZWPin, RST_PIN_VAL );
}

void ResetZW::resetPinRelease() {
    FUNC_LOGD( "Zwave module reset pin release" );
    gpio_set_level( resetZWPin, !RST_PIN_VAL );
}

}  // namespace hard
