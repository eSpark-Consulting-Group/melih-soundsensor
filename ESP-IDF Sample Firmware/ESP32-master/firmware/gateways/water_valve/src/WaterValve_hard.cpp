
#include <freertos/FreeRTOS.h>

#include "driver/gpio.h"
#include "string.h"
#include <stdio.h>
#include <freertos/task.h>
#include "WaterValve_hard.h"
#include "WaterValveManager.h"

#include "driver/ledc.h"

void init_pwm() {
    ledc_timer_config_t ledc_timer;
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num = LEDC_TIMER_1;
    ledc_timer.duty_resolution = LEDC_TIMER_10_BIT;
    ledc_timer.freq_hz = 900;  // set output frequency at 900 Hz
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config( &ledc_timer );

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel;
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel = LEDC_CHANNEL_1;
    ledc_channel.timer_sel = LEDC_TIMER_1;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num = (gpio_num_t)CONFIG_IO_STEP_M;
    ledc_channel.duty = 100;  // set duty at about 10%
    ledc_channel.hpoint = 0;
    ledc_channel_config( &ledc_channel );
    ledc_bind_channel_timer( LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, LEDC_TIMER_1 );
}

#define TIME_WORK_MOTOR 18 * 20

static uint32_t state_water_mash, state_water_mash_upd = 0;
static uint8_t state_mash = WaterValve_hard::IDL_W;
static uint8_t direction_w = 0;  // 0 - bit dir (0-open,1-close) // 1 - bit motor state (0-stop, 1 - work)

WaterValve_hard::~WaterValve_hard() {}

WaterValve_hard::WaterValve_hard() {}

WaterValve_hard& WaterValve_hard::get() {
    static WaterValve_hard water;
    return water;
}

void WaterValve_hard::inst_input_button( gpio_num_t gpioPin ) {
    gpio_pad_select_gpio( gpioPin );
    gpio_set_direction( gpioPin, GPIO_MODE_INPUT );
    gpio_intr_disable( gpioPin );
    return;
}

void WaterValve_hard::init_water() {
    // init LED
    state_water_mash = 0;
#ifdef IS_FACTORY_TEST
    inst_out_button( WaterValve_hard::EN_M, 1 );
    inst_out_button( WaterValve_hard::DIR_M, 1 );
#else
    inst_out_button( WaterValve_hard::LED_OPEN, 1 );
    inst_out_button( WaterValve_hard::LED_CLOSE, 1 );

    inst_out_button( WaterValve_hard::EN_M, 1 );
    inst_out_button( WaterValve_hard::DIR_M, 1 );

    inst_input_button( WaterValve_hard::BUTTON_OPEN );
    inst_input_button( WaterValve_hard::BUTTON_CLOSE );
    inst_input_button( WaterValve_hard::HALL_OPEN );
    inst_input_button( WaterValve_hard::HALL_CLOSE );

    xTaskCreate( water_data, "get_water_data", 1024 * 2, NULL, 7, NULL );
    xTaskCreate( motor_handler, "motor handler", 1024 * 2, NULL, 7, NULL );
#endif
}

void WaterValve_hard::stop_motor( uint8_t state_m ) {
    gateways::water_valve::WaterValveManager::get().onUpdateValveState(
        state_m == OPEN_W ? true : state_m == CLOSE_W ? false : 2 );
    gpio_set_level( EN_M, 1 );
    state_mash = state_m;
    direction_w = 0;

    ledc_stop( LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0 );
    printf( "STOP motor. State - %s\r\n", state_m == OPEN_W ? "CLOSE" : state_m == CLOSE_W ? "OPEN" : "ERROR" );
}

void WaterValve_hard::start_motor( uint8_t dir ) {
    printf( "Start motor. Direction - %d\r\n", dir );

#ifndef IS_FACTORY_TEST
    gpio_set_level( WaterValve_hard::LED_CLOSE, dir ? 0 : 1 );
    gpio_set_level( WaterValve_hard::LED_OPEN, dir ? 1 : 0 );
#endif

    if( !dir )
        gpio_set_level( DIR_M, 0 );
    else
        gpio_set_level( DIR_M, 1 );

    init_pwm();
    gpio_set_level( EN_M, 0 );
}

void WaterValve_hard::water_data( void* arg ) {
    uint8_t st_hall = 0;
    uint16_t work_count = 0;
    uint8_t last_state_HOL_OP = 1;
    uint8_t last_state_HOL_CL = 1;

    vTaskDelay( pdMS_TO_TICKS( 500 ) );

    st_hall = gpio_get_level( HALL_OPEN );
    gpio_set_level( WaterValve_hard::LED_OPEN, st_hall );
    state_mash = ERROR_W;
    if( !st_hall ) {
        state_mash = OPEN_W;
        gateways::water_valve::WaterValveManager::get().onUpdateValveState( true );
    }

    st_hall = gpio_get_level( HALL_CLOSE );
    gpio_set_level( WaterValve_hard::LED_CLOSE, st_hall );
    if( !st_hall ) {
        state_mash = CLOSE_W;
        gateways::water_valve::WaterValveManager::get().onUpdateValveState( false );
    }

    while( state_mash != 0xcd ) {
        vTaskDelay( pdMS_TO_TICKS( 50 ) );

        state_water_mash = 0;
        if( !direction_w ) {
            if( ( state_mash == OPEN_W ) || ( state_mash == CLOSE_W ) ) {
                if( ( gpio_get_level( HALL_OPEN ) ) && ( gpio_get_level( HALL_CLOSE ) ) ) {
                    state_mash = ERROR_W;
                }
            }
        }

        uint8_t st_gp = gpio_get_level( HALL_OPEN );
        if( state_mash == ERROR_W ) {
            if( !st_gp ) {
                state_water_mash = HALL_OPEN;
                last_state_HOL_OP = st_gp;
                goto l1;
            }
            if( !gpio_get_level( HALL_CLOSE ) ) {
                state_water_mash = HALL_CLOSE;
                last_state_HOL_CL = 0;
                goto l1;
            }
        }
        if( !st_gp && st_gp != last_state_HOL_OP ) {
            if( state_mash != OPEN_W ) {
                state_water_mash = HALL_OPEN;
                last_state_HOL_OP = st_gp;
                goto l1;
            }
        }
        else {
            if( ( state_mash != OPENING_W ) && ( state_mash != OPEN_W ) ) {
                if( !gpio_get_level( BUTTON_OPEN ) ) {
                    state_water_mash = BUTTON_OPEN;
                }
            }
        }
        last_state_HOL_OP = st_gp;
        st_gp = gpio_get_level( HALL_CLOSE );
        if( !st_gp && st_gp != last_state_HOL_CL ) {
            if( state_mash != CLOSE_W ) {
                state_water_mash = HALL_CLOSE;
                last_state_HOL_CL = st_gp;
                goto l1;
            }
        }
        else {
            if( ( state_mash != CLOSING_W ) && ( state_mash != CLOSE_W ) ) {
                if( !gpio_get_level( BUTTON_CLOSE ) ) {
                    state_water_mash = BUTTON_CLOSE;
                }
            }
        }
        last_state_HOL_CL = st_gp;

    l1:
        if( !direction_w ) {
            st_gp = gpio_get_level( HALL_CLOSE );
            if( !st_gp ) {
                gpio_set_level( WaterValve_hard::LED_CLOSE, 0 );
                gpio_set_level( WaterValve_hard::LED_OPEN, 1 );
            }

            st_gp = gpio_get_level( HALL_OPEN );
            if( !st_gp ) {
                gpio_set_level( WaterValve_hard::LED_CLOSE, 1 );
                gpio_set_level( WaterValve_hard::LED_OPEN, 0 );
            }
        }

        if( state_water_mash_upd ) {
            state_water_mash = state_water_mash_upd;
            state_water_mash_upd = 0;
        }

        if( state_water_mash == 0xfe )
            break;
        switch( state_water_mash ) {
        case HALL_OPEN: {
            st_hall = gpio_get_level( HALL_OPEN );
            if( work_count == 0 )
                st_hall = 0;
            state_water_mash = 0;

            if( ( !st_hall ) && ( state_mash == OPENING_W ) ) {
                stop_motor( OPEN_W );
            }
            else {
                if( !st_hall && state_mash != OPEN_W ) {
                    stop_motor( OPEN_W );
                }
            }
            gpio_set_level( WaterValve_hard::LED_OPEN, st_hall );
            gpio_set_level( WaterValve_hard::LED_CLOSE, 1 );

            break;
        }
        case HALL_CLOSE: {
            st_hall = gpio_get_level( HALL_CLOSE );
            if( work_count == 0 )
                st_hall = 0;
            state_water_mash = 0;

            if( ( !st_hall ) && ( state_mash == CLOSING_W ) ) {
                stop_motor( CLOSE_W );
            }
            else {
                if( !st_hall && state_mash != CLOSE_W ) {
                    stop_motor( CLOSE_W );
                }
            }
            gpio_set_level( WaterValve_hard::LED_CLOSE, st_hall );
            gpio_set_level( WaterValve_hard::LED_OPEN, 1 );

            break;
        }
        case BUTTON_OPEN: {
            if( state_mash == OPEN_W )
                break;
            printf( "BUTTON_CLOSE pres\r\n" );
            state_mash = OPENING_W;
            direction_w = 2;
            state_water_mash = 0;
            work_count = 1;
            start_motor( 0 );
            break;
        }
        case BUTTON_CLOSE: {
            if( state_mash == CLOSE_W )
                break;
            printf( "BUTTON_OPEN pres\r\n" );
            state_mash = CLOSING_W;
            direction_w = 3;
            state_water_mash = 0;
            work_count = 1;
            start_motor( 1 );
            break;
        }
        }
        if( ( state_mash == CLOSING_W ) || ( state_mash == OPENING_W ) ) {
            work_count++;
            if( work_count > TIME_WORK_MOTOR ) {
                state_water_mash = 0;
                work_count = 0;
                stop_motor( ERROR_W );
            }
        }
    }
}

void WaterValve_hard::inst_out_button( gpio_num_t gpioPin, uint8_t state ) {
    gpio_pad_select_gpio( gpioPin );
    gpio_set_direction( gpioPin, GPIO_MODE_OUTPUT );
    gpio_set_level( gpioPin, state );
}

void WaterValve_hard::motor_handler( void* arg ) {
    static uint8_t blink = 0, blink_st = 0, blink_e = 0, blink_st_e = 0;
    vTaskDelay( pdMS_TO_TICKS( 400 ) );
    while( state_mash != 0xcd ) {
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
        while( direction_w ) {
            vTaskDelay( pdMS_TO_TICKS( 50 ) );
            if( blink < 4 ) {
                blink++;
            }
            else {
                blink = 0;
                blink_st = blink_st ? 0 : 1;
                if( direction_w == 2 ) {
                    gpio_set_level( WaterValve_hard::LED_OPEN, blink_st );
                }
                else {
                    gpio_set_level( WaterValve_hard::LED_CLOSE, blink_st );
                }
            }
        }

        if( !direction_w ) {
            if( state_mash == ERROR_W ) {
                if( blink_e < 4 ) {
                    blink_e++;
                }
                else {
                    blink_e = 0;
                    blink_st_e = blink_st_e ? 0 : 1;
                    gpio_set_level( WaterValve_hard::LED_OPEN, blink_st_e );
                    gpio_set_level( WaterValve_hard::LED_CLOSE, blink_st_e );
                }
            }
            else {
                blink = 0;
                blink_st = 0;
                blink_e = 0;
                blink_st_e = 0;
            }
        }
    }
}

void WaterValve_hard::set_valve( uint8_t value ) {
    if( value == 1 ) {
        if( ( state_mash != OPENING_W ) && ( state_mash != OPEN_W ) ) {
            state_water_mash_upd = BUTTON_OPEN;
        }
    }
    else {
        if( ( state_mash != CLOSING_W ) && ( state_mash != CLOSE_W ) ) {
            state_water_mash_upd = BUTTON_CLOSE;
        }
    }
}