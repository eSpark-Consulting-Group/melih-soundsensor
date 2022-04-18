//
// Created by samogon on 09.12.19.
//
#pragma once
#include <freertos/FreeRTOS.h>
#include "driver/gpio.h"
#include <freertos/timers.h>
#include <esp_timer.h>

class WaterValve_hard {
public:
    enum state_water : uint8_t {
        IDL_W = 0,
        OPEN_W = 1,
        CLOSE_W = 2,
        OPENING_W = 3,
        CLOSING_W = 4,
        ERROR_W = 0xff,
    };

    ~WaterValve_hard();
    WaterValve_hard();
    static WaterValve_hard& get();
    void init_water();
    void set_valve( uint8_t value );

    static void stop_motor( uint8_t state_m );
    static void start_motor( uint8_t dir );

private:
    static constexpr auto LED_OPEN = (gpio_num_t)CONFIG_IO_LED_OPEN;
    static constexpr auto LED_CLOSE = (gpio_num_t)CONFIG_IO_LED_CLOSE;
    static constexpr auto BUTTON_OPEN = (gpio_num_t)CONFIG_IO_BUTTON_OPEN;
    static constexpr auto BUTTON_CLOSE = (gpio_num_t)CONFIG_IO_BUTTON_CLOSE;
    static constexpr auto HALL_OPEN = (gpio_num_t)CONFIG_IO_HALL_OPEN;
    static constexpr auto HALL_CLOSE = (gpio_num_t)CONFIG_IO_HALL_CLOSE;
    static constexpr auto STEP_M = (gpio_num_t)CONFIG_IO_STEP_M;
    static constexpr auto DIR_M = (gpio_num_t)CONFIG_IO_DIR_M;
    static constexpr auto EN_M = (gpio_num_t)CONFIG_IO_EN_M;

    static void motor_handler( void* arg );

    void inst_input_button( gpio_num_t gpioPin );
    void inst_out_button( gpio_num_t gpioPin, uint8_t state );
    static void water_data( void* arg );
    static void periodic_timer_callback( void* arg );
};