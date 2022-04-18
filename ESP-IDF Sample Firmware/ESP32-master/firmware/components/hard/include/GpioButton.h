#pragma once

#include <cstdint>
#include <functional>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "driver/gpio.h"
#include <esp_timer.h>

#include "Bus.h"

namespace hard {

class GpioButton : public ::bus::IBusClient {
public:
    enum class ButtonState { ButtonReleased, ButtonPressed };

    using GpioPin = gpio_num_t;

    static constexpr auto DEBOUNCE_TIME_MS = 20;
    static constexpr auto ACTION_TIMEOUT_MS = 1000;
    static constexpr auto LONG_CLICK_MS = 5000;
    static constexpr auto VERY_LONG_CLICK_MS = 10000;
    static constexpr auto VERY_VERY_LONG_CLICK_MS = 20000;
    static constexpr auto START_TIMER_TIMEOUT_MS = 50;

    GpioButton( const GpioPin gpioPin );
    ~GpioButton();

    void update();

private:
    static void IRAM_ATTR gpio_isr_handler( GpioButton* self );
    static void IRAM_ATTR debounce_handler( TimerHandle_t handle );
    static void IRAM_ATTR action_handler( TimerHandle_t handle );
    ButtonState IRAM_ATTR readButton();
    void IRAM_ATTR setAction();
    void reset();

private:
    GpioPin m_gpioPin;
    TimerHandle_t m_debounceTimer;
    TimerHandle_t m_actionTimer;
    int m_nClicks;
    TickType_t m_clickStart;
    TickType_t m_lastClickDuration;
    ButtonState m_buttonState;
    std::unique_ptr<::bus::IBusCommand> m_event;
};

}  // end of namespace hard
