#pragma once
#ifdef CONFIG_BOARD_TYPE_PLUG_HUB
#include "nvs_flash.h"
#include "nvs.h"
#include <driver/gpio.h>

class Meter_cc {
public:
    // #define DEBUG_M
    // #define LAMP
    // #define NOT_CALIB

    typedef struct {
        float U;
        float A;
        float P;
        double Wt_h;
        float KWt_h;
    } Meter_values_t;

    static constexpr auto RELAY_A_PIN = (gpio_num_t)CONFIG_IO_RELAY_A_PIN;
#ifndef CONFIG_METER_CHIP_STPM
    enum state_ht : uint8_t {
        RMS_U = 0x08,
        POWER_P1 = 0x0A,
        CHIP_ID = 0x1B,
        DEV_ID = 0x1C,
        SET_REG = 0x25,
        IDL = 0xFF
    };
#else
    //    static constexpr auto RELAY_A_PIN = CONFIG_IO_RELAY_A_PIN;
    static constexpr auto RELAY_B_PIN = (gpio_num_t)CONFIG_IO_RELAY_B_PIN;

    enum state_ht : uint8_t {
        DSP_CR1 = 0x00,
        DSP_CR3 = 0x04,
        DSP_CR5 = 0x08,
        DSP_CR6 = 0x0A,
        DSP_CR9 = 0x10,
        DSP_CR10 = 0x12,
        DFE_CR1 = 0x18,
        RMS = 0x48,
        POWER_P1 = 0x62,
        ENERGI_1 = 0x8A,
        IDL = 0xFF
    };

#endif

    static Meter_values_t* GetMeter();
    static void Meter_reset();
    static void Meter_set_consumed_energy_meter( float KWt_h );

    static Meter_cc& get();
    void init_uart_meter();
    void init_meter();

    Meter_cc();
    ~Meter_cc();

    static constexpr auto RX_BUF_SIZE = 130;

private:
    static constexpr auto TXD_PIN = (gpio_num_t)CONFIG_IO_TXD_PIN;
    static constexpr auto RXD_PIN = (gpio_num_t)CONFIG_IO_RXD_PIN;
    static constexpr auto RXD1_PIN = (gpio_num_t)CONFIG_IO_RXD1_PIN;
#ifdef CONFIG_METER_CHIP_STPM

    static constexpr auto TXD1_PIN = (gpio_num_t)CONFIG_IO_TXD1_PIN;

    static uint32_t rx_task2();
    static uint32_t sendData2( uint8_t read_addr, uint8_t write_addr, uint8_t LSB_data, uint8_t MSB_data );
    static void init_uart1_for_calibr();
    static uint32_t get_meter();
    static void init_stpm();
    static uint8_t revers( uint8_t n );
    static uint8_t CRC8( uint8_t* d1 );
    //    static uint32_t old_power_dan;

#else

    static constexpr auto RST_PIN = (gpio_num_t)CONFIG_IO_RST_PIN;

    static int sendData2( uint8_t* data, int len );
    static void rx_task2();
    static void pars_meter_data( uint8_t* data, const int rxBytes );

    static void rx_task1();
    void init_uart1_for_calibr();

#endif
    static void get_meter_data( void* arg );
    void initialize_nvs();
};
#endif
