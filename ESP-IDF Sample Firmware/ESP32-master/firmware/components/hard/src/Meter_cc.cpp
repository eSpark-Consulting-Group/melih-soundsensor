
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "string.h"
#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"

#include <esp_timer.h>
#include <Meter_cc.h>

#ifdef CONFIG_BOARD_TYPE_PLUG_HUB

#define U_CALIB UART_NUM_2
#define U_WORK UART_NUM_1

static nvs_handle my_handle;
static uint8_t per_state;
static uint8_t tm_st;
static double kx;
static double b;
static uint8_t kor_ph;
static uint8_t kor_pl;
#ifndef CONFIG_METER_CHIP_STPM
static uint8_t start_calibrate = 0;
#else
static uint32_t old_vats = 0;
#endif

static Meter_cc::Meter_values_t meter_data;

Meter_cc::Meter_values_t* Meter_cc::GetMeter() {
    return &meter_data;
}

Meter_cc::~Meter_cc() {}

#ifndef CONFIG_METER_CHIP_STPM
int Meter_cc::sendData2( uint8_t* data, int len ) {
    const int txBytes = uart_write_bytes( U_WORK, (const char*)data, len );
    return txBytes;
}

void Meter_cc::pars_meter_data( uint8_t* data, const int rxBytes ) {
    if( rxBytes < 3 )
        return;
    int32_t dan = ( data[0] << 16 ) + ( data[1] << 8 ) + data[2];
    if( data[0] >= 0x80 )
        dan = dan + ( 0xff << 24 );
    if( per_state == RMS_U ) {
        meter_data.U = (float)dan;
        meter_data.U = ( meter_data.U / 10000 );
        meter_data.U = ( meter_data.U - b ) / kx;
    }
    else {
        if( per_state == POWER_P1 ) {
            if( dan < 0 )
                dan = 0;
            meter_data.P = (float)dan;
            meter_data.P = meter_data.P / 10;
            meter_data.A = meter_data.P / meter_data.U;
            meter_data.Wt_h = meter_data.Wt_h + ( meter_data.P / 3600 );
            meter_data.KWt_h = meter_data.Wt_h / 1000;
#ifdef DEBUG_M
            printf( "\r\n[%d] U=%f, A=%f, P=%f, W_h=%f\r\n",
                    esp_log_timestamp(),
                    meter_data.U,
                    meter_data.A,
                    meter_data.P,
                    meter_data.Wt_h );
#endif
        }
    }
}

void Meter_cc::get_meter_data( void* arg ) {
    static uint8_t data[2] = { 0x6A, RMS_U };
    static uint64_t delay_tm = ( ( esp_timer_get_time() / 1000000 ) + 1 ) * 1000000;
    delay_tm = delay_tm - esp_timer_get_time();
    while( 1 ) {
        vTaskDelay( pdMS_TO_TICKS( delay_tm / 1000 ) );

        if( data[0] == 0xfe )
            break;
        sendData2( &data[0], 2 );
        per_state = data[1];
        if( data[1] == RMS_U ) {
            data[1] = POWER_P1;
        }
        else {
            data[1] = RMS_U;
        }
        rx_task2();
        uint64_t next_tm = ( ( esp_timer_get_time() / 1000000 ) * 1000000 ) + 500000;
        if( next_tm < esp_timer_get_time() )
            next_tm = next_tm + 500000;
        delay_tm = next_tm - esp_timer_get_time();
    }
}

void Meter_cc::rx_task2() {
    uint8_t data[24];

    if( per_state != IDL ) {
        const int rxBytes = uart_read_bytes( U_WORK, data, 24, 50 );
        if( rxBytes > 0 ) {
            if( per_state != IDL ) {
                uint8_t crc = 0x6a + (uint8_t)per_state;
                data[rxBytes] = 0;
#ifdef DEBUG_M
                printf( "\r\nData from HT:\r\n" );
#endif
                for( int i = 0; i < rxBytes; i++ ) {
#ifdef DEBUG_M
                    printf( "0x%02X ", data[i] );
#endif
                    if( i < 3 )
                        crc = crc + data[i];
                }
                if( rxBytes > 1 ) {
                    crc = crc ^ 0xff;
                    if( crc != data[3] ) {
#ifdef DEBUG_M
                        printf( "CRC ERROR" );
#endif
                        uart_read_bytes( U_WORK, data, 24, 50 );
                        per_state = IDL;
                        return;
                    }
                    else {
                        pars_meter_data( data, rxBytes );
                    }
                }
#ifdef DEBUG_M
                printf( "\r\n" );
#endif
            }
            per_state = IDL;
        }
    }
}

void Meter_cc::init_uart_meter() {
    uart_config_t uart_config;
    uart_config.baud_rate = 4800;
    uart_config.parity = UART_PARITY_EVEN;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;

    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_param_config( U_WORK, &uart_config );
    uart_set_pin( U_WORK, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE );

    uart_driver_install( U_WORK, RX_BUF_SIZE, 0, 0, NULL, 0 );
}

void Meter_cc::rx_task1() {
    esp_err_t err;
    uint8_t data[24];
    vTaskDelay( pdMS_TO_TICKS( 20 ) );
    const int rxBytes = uart_read_bytes( U_CALIB, data, 24, 0 );
    if( rxBytes > 0 ) {
        data[rxBytes] = 0;
        printf( "DATA UART1: 0X%02X\r\n", data[0] );
        if( ( start_calibrate == 0 ) && ( data[0] == 0x97 ) ) {
            data[0] = 0x6A;
            data[1] = 0x1C;
            sendData2( data, 2 );
            start_calibrate = 1;
            return;
        }

        if( data[0] == 0x6a ) {
            sendData2( data, rxBytes );
            per_state = data[1];
        }
        else {
            if( data[0] == 0x3f ) {
                int32_t kx_i;
                int32_t b_i;

                kx_i = ( data[1] << 24 );
                kx_i = kx_i + ( data[2] << 16 );
                kx_i = kx_i + ( data[3] << 8 );
                kx_i = kx_i + data[4];

                b_i = ( data[5] << 24 );
                b_i = b_i + ( data[6] << 16 );
                b_i = b_i + ( data[7] << 8 );
                b_i = b_i + data[8];

                kx = (double)kx_i;
                b = (double)b_i;
                kx = kx / 10000;
                b = b / 10000;
                kor_ph = data[9];
                kor_pl = data[10];
                uint16_t calib_on = 0x55;  // value will default to 0, if not set yet in NVS
                err = nvs_set_u16( my_handle, "calib_on", calib_on );

                err = nvs_set_i32( my_handle, "calib_kx", kx_i );
                err = nvs_set_i32( my_handle, "calib_b", b_i );
                err = nvs_set_u8( my_handle, "calib_ph", kor_ph );
                err = nvs_set_u8( my_handle, "calib_pl", kor_pl );
                err = nvs_commit( my_handle );
                printf( ( err != ESP_OK ) ? "Failed!\r\n" : "Done\r\n" );
                start_calibrate = 2;
            }
        }
    }
}

void Meter_cc::init_uart1_for_calibr() {
    uart_config_t uart_config;
    uart_config.baud_rate = 4800;
    uart_config.parity = UART_PARITY_EVEN;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;

    uart_config.stop_bits = UART_STOP_BITS_1;

    uart_param_config( U_CALIB, &uart_config );
    uart_set_pin( U_CALIB, UART_PIN_NO_CHANGE, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE );

    uart_driver_install( U_CALIB, RX_BUF_SIZE, 0, 0, NULL, 0 );
}

void Meter_cc::initialize_nvs() {
    printf( "Opening Non-Volatile nvs (NVS) handle... " );
    esp_err_t err = nvs_open( "nvs", NVS_READWRITE, &my_handle );
    if( err != ESP_OK ) {
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
        printf( "Error (%s) opening NVS handle!\n", esp_err_to_name( err ) );
#else   // not CONFIG_ESP_ERR_TO_NAME_LOOKUP
        printf( "Error (%d) opening NVS handle!\n", err );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
    }
    else {
        printf( "Done\r\n" );

        printf( "Reading Calibr data from NVS ... " );
        uint16_t calib_on = 0;  // value will default to 0, if not set yet in NVS
        err = nvs_get_u16( my_handle, "calib_on", &calib_on );
        switch( err ) {
        case ESP_OK:
            printf( "Done\r\n" );
            int32_t kx_i;
            int32_t b_i;

            err = nvs_get_i32( my_handle, "calib_kx", &kx_i );
            err = nvs_get_i32( my_handle, "calib_b", &b_i );
            err = nvs_get_u8( my_handle, "calib_ph", &kor_ph );
            err = nvs_get_u8( my_handle, "calib_pl", &kor_pl );
            kx = (double)kx_i;
            b = (double)b_i;
            kx = kx / 10000;
            b = b / 10000;

            printf( "Kx - %f, b - %f, ph - 0X%02X, pl - 0X%02X\r\n", kx, b, kor_ph, kor_pl );

            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf( "METER NOT CALIBRATION!\r\nINTI UART1\r\nSTART CALIBRATION!\r\n" );
            break;
        default:
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
            printf( "Error (%s) reading!\r\n", esp_err_to_name( err ) );
#else   // not CONFIG_ESP_ERR_TO_NAME_LOOKUP
            printf( "Error (%d) reading!\r\n", err );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
        }
    }
}

void Meter_cc::init_meter() {
    per_state = IDL;
    meter_data = { 0.0001, 0.0001, 0.001, 0.0001, 0.00001 };
    kx = 1.00000000001;
    b = 0.0000001;
    kor_ph = 0;
    kor_pl = 0;
    tm_st = 0;
    Meter_reset();
    GetMeter();

    gpio_pad_select_gpio( RXD1_PIN );
    gpio_set_direction( RXD1_PIN, GPIO_MODE_INPUT );
    gpio_pullup_dis( RXD1_PIN );

    gpio_pad_select_gpio( RST_PIN );
    gpio_set_direction( RST_PIN, GPIO_MODE_OUTPUT );
    gpio_set_level( RST_PIN, 255 );
    ets_delay_us( 300000 );

    initialize_nvs();
    init_uart_meter();
    init_uart1_for_calibr();

    gpio_pullup_dis( RXD1_PIN );
#ifndef NOT_CALIB

    uint16_t cou = 0;
    while( cou < 100 ) {
        rx_task1();
        if( start_calibrate ) {
            //            gpio_set_level( RELAY_A_PIN, 255 );
            gpio_pad_select_gpio( RELAY_A_PIN );
            gpio_set_direction( RELAY_A_PIN, GPIO_MODE_INPUT );
            gpio_pullup_dis( RELAY_A_PIN );
            break;
        }
        cou++;
    }

    while( start_calibrate == 1 ) {
        rx_task1();
        rx_task2();
    }
#endif

    gpio_pad_select_gpio( RELAY_A_PIN );
    gpio_set_direction( RELAY_A_PIN, GPIO_MODE_OUTPUT );

    uart_driver_delete( U_CALIB );

    uint8_t str1[5] = { 0x6a, 0xB2, 0x00, 0xA6, 0x3D };
    sendData2( &str1[0], 5 );
    vTaskDelay( pdMS_TO_TICKS( 200 ) );
    if( ( kor_ph == kor_pl ) && ( kor_ph == 0 ) ) {
        vTaskDelay( pdMS_TO_TICKS( 20 ) );
    }
    else {
        str1[1] = 0xD0;
        str1[2] = kor_ph;
        str1[3] = kor_pl;
        uint8_t crc = str1[0] + str1[1] + str1[2] + str1[3];
        crc = crc ^ 0xff;
        str1[4] = crc;
        sendData2( &str1[0], 5 );
        vTaskDelay( pdMS_TO_TICKS( 200 ) );
    }
    xTaskCreate( get_meter_data, "get_meter_data", 1024 * 2, NULL, configMAX_PRIORITIES, NULL );
}
#else

static int32_t CHV_d = 0xF800;
static int32_t CHC_d = 0xF800;
static int32_t OFS_d = 0;
static double whats = 0;

uint8_t Meter_cc::revers( uint8_t n ) {
    n = ( ( n >> 1 ) & 0x55 ) | ( ( n << 1 ) & 0xaa );
    n = ( ( n >> 2 ) & 0x33 ) | ( ( n << 2 ) & 0xcc );
    n = ( ( n >> 4 ) & 0x0F ) | ( ( n << 4 ) & 0xF0 );
    return n;
}

uint8_t Meter_cc::CRC8( uint8_t* d1 ) {
    uint8_t d[5];
    for( int i = 0; i < 4; i++ ) {
        d[i] = revers( d1[i] );
    }
    uint8_t crc = 0x0;
    uint8_t l = 0;
    while( l < 4 ) {
        uint8_t loc_u8Idx = 0;
        uint8_t loc_u8Temp = 0;
        uint8_t u8Data = d[l];
        while( loc_u8Idx < 8 ) {
            loc_u8Temp = u8Data ^ crc;
            crc <<= 1;
            if( loc_u8Temp & 0x80 ) {
                crc ^= 0x07;
            }
            u8Data <<= 1;
            loc_u8Idx++;
        }
        l++;
    }
    return revers( crc );
}

uint32_t Meter_cc::sendData2( uint8_t read_addr, uint8_t write_addr, uint8_t LSB_data, uint8_t MSB_data ) {
    uint8_t data[5];
    data[0] = read_addr;
    data[1] = write_addr;
    data[2] = LSB_data;
    data[3] = MSB_data;
    data[4] = CRC8( data );
    uart_write_bytes( U_WORK, (const char*)data, 5 );
    return rx_task2();
}

void Meter_cc::init_uart_meter() {
    uart_config_t uart_config;

    uart_config.baud_rate = 9600;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;

    uart_param_config( U_WORK, &uart_config );
    uart_set_pin( U_WORK, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE );

    uart_driver_install( U_WORK, RX_BUF_SIZE, 0, 0, NULL, 0 );
}

void Meter_cc::init_uart1_for_calibr() {
    char* data = (char*)malloc( 250 );

    uart_config_t uart_config;
    uart_config.baud_rate = 460800;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;

    uart_param_config( U_CALIB, &uart_config );
    uart_set_pin( U_CALIB, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE );

    uart_driver_install( U_CALIB, RX_BUF_SIZE, RX_BUF_SIZE, 0, NULL, 0 );
    vTaskDelay( pdMS_TO_TICKS( 20 ) );

    sprintf( data, "Init Calibration" );
    printf( "%s\r\n", data );
    uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );
    vTaskDelay( pdMS_TO_TICKS( 20 ) );
    data[0] = 0;
    int len = uart_read_bytes( U_CALIB, (uint8_t*)data, 17, 200 );

    if( len ) {
        data[len] = 0;
        if( !strcasecmp( data, "Calibration START" ) ) {
            printf( "%s\r\n", data );
            size_t size_u;
            sprintf( data, "Wait data" );
            uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );
            uint8_t cou = 0;
            while( cou < 50 ) {
                uart_get_buffered_data_len( U_CALIB, &size_u );
                if( size_u < 20 ) {
                    cou++;
                    vTaskDelay( pdMS_TO_TICKS( 100 ) );
                }
                else {
                    len = uart_read_bytes( U_CALIB, (uint8_t*)data, size_u + 20, 20 );
                    data[len] = 0;
                    printf( "Data - %s \r\n", data );
                    if( strstr( data, "DATA" ) ) {
                        vTaskDelay( pdMS_TO_TICKS( 50 ) );
                        char* U = strstr( data, "U=" );
                        char* I = strstr( data, "I=" );
                        if( U && I ) {
                            U = U + 2;
                            I = I + 2;
                            char* tmp = strstr( U, "V" );
                            tmp[0] = 0;
                            tmp = strstr( I, "A" );
                            tmp[0] = 0;
                            float Un = strtof( U, &tmp );
                            float In = strtof( I, &tmp );
                            printf( "U=%f I=%f\r\n", Un, In );
                            float Xv = (float)( Un * 27.711986049 );
                            float Xi = (float)( In * 2140.842666667 );

                            gpio_set_level( Meter_cc::RELAY_A_PIN, 1 );
                            gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );
                            vTaskDelay( pdMS_TO_TICKS( 50 ) );
                            gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
                            gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );
                            vTaskDelay( pdMS_TO_TICKS( 800 ) );

                            sprintf( data, "CALIBRATION DATA Xv = %f   Xi = %f \r\n", Xv, Xi );
                            printf( "%s", data );
                            uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );

                            uint32_t CHV = 0;
                            uint32_t CHC = 0;
                            int32_t POW = 0;
                            sendData2( DSP_CR5, DSP_CR5, 0x00, 0xf8 );
                            vTaskDelay( pdMS_TO_TICKS( 50 ) );
                            sendData2( DSP_CR6, DSP_CR6, 0x00, 0xf8 );
                            vTaskDelay( pdMS_TO_TICKS( 50 ) );
                            sendData2( DSP_CR10, DSP_CR10 + 1, 0x00, 0x00 );
                            vTaskDelay( pdMS_TO_TICKS( 50 ) );

                            sendData2( RMS, IDL, 0, 0 );
                            for( cou = 0; cou < 10; cou++ ) {
                                vTaskDelay( pdMS_TO_TICKS( 500 ) );
                                uint32_t dan = sendData2( POWER_P1, IDL, 0, 0 );
                                uint32_t dan_U = dan & 0x7fff;
                                uint32_t dan_I = dan >> 15;
                                CHV = CHV + dan_U;
                                CHC = CHC + dan_I;
                                dan = sendData2( RMS, IDL, 0, 0 );
                                if( dan > 0x7ffffff ) {
                                    int32_t p = dan << 4;
                                    p = p / 16;
                                    POW = POW + p;
                                }
                                else {
                                    POW = POW + dan;
                                }
                                //                                float dan_P  = 0.000224743;
                                //                                dan_P = (float)( meter_data.P * dan ); // 0,000898972
                                //                                LSBpo

                                // printf( "dan_U - %d  dan_I - %d POW - %d\r\n", dan_U, dan_I, POW );
                            }
                            CHV = CHV / 10;
                            CHC = CHC / 10;
                            //                            POW = POW / 10 / ( -4 );
                            POW = 0;
                            CHV = ( uint32_t )( 14336 * Xv / CHV ) - 12288;
                            CHC = ( uint32_t )( 14336 * Xi / CHC ) - 12288;
                            printf( "SRED dan_U - %d  dan_I - %d POW - %d\r\n", CHV, CHC, POW );
                            if( CHV >= ( 1 << 13 ) ) {
                                sprintf( data, "CHV ERROR\r\n" );
                                // printf( "%s", data );
                                uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );
                            }
                            if( CHC >= ( 1 << 13 ) ) {
                                sprintf( data, "CHC ERROR\r\n" );
                                uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );
                            }
                            Xv = 0.125 * (float)( (float)CHV / (float)2048.0 ) + 0.75;
                            Xi = 0.125 * (float)( (float)CHC / (float)2048.0 ) + 0.75;
                            sprintf( data,
                                     "CALIBRATION DATA Kv = %f   Ki = %f  CHV = 0x%x  CHC = 0x%x\r\n",
                                     Xv,
                                     Xi,
                                     CHV,
                                     CHC );
                            printf( "%s\r\n", data );
                            uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );
                            sendData2( DSP_CR3, DSP_CR3 + 1, 0x00, 0x03 );  // S/W_AUTO_Latch OFF LED_OFF
                            vTaskDelay( pdMS_TO_TICKS( 50 ) );
                            uint8_t lsb = CHV & 0xff;
                            uint8_t msb = ( CHV >> 8 ) & 0x0f;
                            sendData2( DSP_CR5, DSP_CR5, lsb, msb );
                            lsb = CHC & 0xff;
                            msb = ( CHC >> 8 ) & 0x0f;
                            sendData2( DSP_CR6, DSP_CR6, lsb, msb );

                            lsb = POW & 0x03;
                            msb = ( POW >> 2 ) & 0xff;
                            lsb = lsb << 6;

                            sendData2( DSP_CR10, DSP_CR10 + 1, lsb << 6, msb );

                            gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
                            gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );

                            vTaskDelay( pdMS_TO_TICKS( 50 ) );
                            gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
                            gpio_set_level( Meter_cc::RELAY_B_PIN, 1 );
                            vTaskDelay( pdMS_TO_TICKS( 50 ) );
                            gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
                            gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );

                            sprintf( data, "Calibration OK" );
                            uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );
                            printf( "%s\r\n", data );

                            vTaskDelay( pdMS_TO_TICKS( 1000 ) );

                            sprintf( data, "Start TEST" );
                            uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );
                            printf( "%s\r\n", data );
                            vTaskDelay( pdMS_TO_TICKS( 50 ) );
                            sendData2( DSP_CR3, DSP_CR3 + 1, 0x80, 0x03 );  // S/W_AUTO_Latch LED_OFF

                            for( cou = 0; cou < 4; cou++ ) {
                                vTaskDelay( pdMS_TO_TICKS( 1000 ) );
                                get_meter();
                                sprintf(
                                    data, "DATA U=%fV I=%fA P=%fWt\r\n", meter_data.U, meter_data.A, meter_data.P );
                                uart_write_bytes( U_CALIB, (const char*)data, strlen( data ) );
                                printf( "%s\r\n", data );
                            }

                            len = uart_read_bytes( U_CALIB, (uint8_t*)data, 20, 20 );
                            data[len] = 0;
                            // printf( "Data - %s \r\n", data );
                            CHC_d = CHC;
                            CHV_d = CHV;

                            uint16_t calib_on = 0x55;  // value will default to 0, if not set yet in NVS
                            esp_err_t err = nvs_set_u16( my_handle, "calib_on", calib_on );

                            err |= nvs_set_i32( my_handle, "calib_CHC", CHC_d );
                            err |= nvs_set_i32( my_handle, "calib_CHV", CHV_d );
                            err |= nvs_set_i32( my_handle, "calib_OFS", POW );
                            if( err ) {
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
                                printf( "Error (%s) reading!\r\n", esp_err_to_name( err ) );
#else   // not CONFIG_ESP_ERR_TO_NAME_LOOKUP
                                printf( "Error (%d)\r\n", err );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
                            }
                            err = nvs_commit( my_handle );
                            if( err ) {
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
                                printf( "Error (%s) reading!\r\n", esp_err_to_name( err ) );
#else   // not CONFIG_ESP_ERR_TO_NAME_LOOKUP
                                printf( "Error (%d)\r\n", err );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
                            }

                            break;
                        }
                    }
                }
            }
        }
    }

    printf( "Calib DONE\r\n" );
    free( data );
    uart_driver_delete( U_CALIB );
    uart_driver_delete( U_WORK );
}

void Meter_cc::initialize_nvs() {
    esp_err_t err = nvs_open( "nvs", NVS_READWRITE, &my_handle );
    if( err != ESP_OK ) {
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
        printf( "Error (%s) \n", esp_err_to_name( err ) );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
    }
    else {
        printf( "Done\r\n" );

        printf( "Reading data\r\n" );
        uint16_t calib_on = 0;  // value will default to 0, if not set yet in NVS
        err = nvs_get_u16( my_handle, "calib_on", &calib_on );

        switch( err ) {
        case ESP_OK:
            printf( "Done\r\n" );
            int32_t kx_i;
            int32_t b_i;

            err = nvs_get_i32( my_handle, "calib_CHC", &kx_i );
            if( err ) {
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
                printf( "Error (%s) reading!\r\n", esp_err_to_name( err ) );
#else   // not CONFIG_ESP_ERR_TO_NAME_LOOKUP
                printf( "Error (%d)\r\n", err );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
                kx_i = CHC_d;
                b_i = CHV_d;
            }
            else {
                err = nvs_get_i32( my_handle, "calib_CHV", &b_i );
                if( err ) {
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
                    printf( "Error (%s) reading!\r\n", esp_err_to_name( err ) );
#else   // not CONFIG_ESP_ERR_TO_NAME_LOOKUP
                    printf( "Error (%d) \r\n", err );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
                    kx_i = CHC_d;
                    b_i = CHV_d;
                }
                else {
                    err = nvs_get_i32( my_handle, "calib_OFS", &OFS_d );
                    if( err ) {
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
                        printf( "Error (%s) reading!\r\n", esp_err_to_name( err ) );
#else   // not CONFIG_ESP_ERR_TO_NAME_LOOKUP
                        printf( "Error (%d)\r\n", err );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
                        OFS_d = 0;
                    }
                }
            }

            CHC_d = kx_i;
            CHV_d = b_i;

            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf( "METER NOT CALIB!\r\n" );
            break;
        default:
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
            printf( "Error (%s) reading!\r\n", esp_err_to_name( err ) );
#else   // not CONFIG_ESP_ERR_TO_NAME_LOOKUP
            printf( "Error (%d)\r\n", err );
#endif  // CONFIG_ESP_ERR_TO_NAME_LOOKUP
        }
    }
}

void Meter_cc::init_meter() {
    per_state = IDL;
    meter_data = { 0.0001, 0.0001, 0.001, 0.0001, 0.00001 };
    kx = 1.00000000001;
    b = 0.0000001;
    kor_ph = 0;
    kor_pl = 0;
    tm_st = 0;

#ifndef LAMP

    gpio_pad_select_gpio( Meter_cc::RELAY_A_PIN );
    gpio_pad_select_gpio( Meter_cc::RELAY_B_PIN );
    gpio_set_direction( Meter_cc::RELAY_B_PIN, GPIO_MODE_OUTPUT );
    gpio_set_direction( Meter_cc::RELAY_A_PIN, GPIO_MODE_OUTPUT );

    vTaskDelay( pdMS_TO_TICKS( 500 ) );
    initialize_nvs();

    init_uart_meter();

    init_stpm();
    // vTaskDelay( pdMS_TO_TICKS( 200 ) );
    init_uart1_for_calibr();
    init_uart_meter();
    xTaskCreate( get_meter_data, "get_meter_data", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL );
#endif
}

void Meter_cc::init_stpm() {
    per_state = IDL;

    sendData2( DSP_CR1, DSP_CR1 + 1, 0x04, 0x04 );  // APM1

    sendData2( DFE_CR1, DFE_CR1 + 1, 0x27, 0x0B );

    uint8_t lsb = CHV_d & 0xff;
    uint8_t msb = ( CHV_d >> 8 ) & 0x0f;
    sendData2( DSP_CR5, DSP_CR5, lsb, msb );
    lsb = CHC_d & 0xff;
    msb = ( CHC_d >> 8 ) & 0x0f;
    sendData2( DSP_CR6, DSP_CR6, lsb, msb );
    lsb = ( OFS_d & 0x03 ) << 6;
    msb = ( OFS_d >> 2 ) & 0xff;
    sendData2( DSP_CR10, DSP_CR10 + 1, lsb, msb );  // OFS1

    sendData2( DSP_CR3, DSP_CR3 + 1, 0x80, 0x03 );  // S/W_AUTO_Latch LED_OFF
    rx_task2();
    vTaskDelay( pdMS_TO_TICKS( 200 ) );
}

uint32_t Meter_cc::get_meter() {
    sendData2( RMS, IDL, 0, 0 );
    uint32_t dan = 0;
    uint8_t count_u = 0;
    while( !dan ) {
        dan = sendData2( RMS, IDL, 0, 0 );
        if( dan == 0x040000A0 ) {
            init_stpm();
            count_u = 10;
        }
        if( count_u++ >= 10 )
            return 0;
    }

    //        per_state = RMS;

    uint32_t dan_U = dan & 0x7fff;
    uint32_t dan_I = dan >> 15;
    meter_data.U = 0.036085469;
    meter_data.U = (float)( meter_data.U * dan_U );
    meter_data.A = 0.000467106;
    meter_data.A = (float)( meter_data.A * dan_I );

    //        per_state = POWER_P1;
    sendData2( POWER_P1, IDL, 0, 0 );
    dan = 0;
    count_u = 0;
    while( !dan ) {
        dan = sendData2( POWER_P1, IDL, 0, 0 );
        if( dan == 0x040000A0 ) {
            init_stpm();
            count_u = 10;
        }
        if( count_u++ >= 10 )
            return 0;
    }

    if( dan > 0x7ffffff ) {
        dan = 0;
    }
    meter_data.P = 0.0002622;
    meter_data.P = (float)( meter_data.P * dan );
    sendData2( ENERGI_1, IDL, 0, 0 );
    dan = 0;
    count_u = 0;
    while( !dan ) {
        dan = sendData2( ENERGI_1, IDL, 0, 0 );
        if( dan == 0x040000A0 ) {
            init_stpm();
            count_u = 10;
        }
        if( count_u++ >= 10 )
            return 0;
    }

    uint32_t wt = 0;
    if( old_vats == 0 )
        old_vats = dan;
    if( dan < old_vats ) {
        wt = 0xffffffff - old_vats + dan;
    }
    else {
        wt = dan - old_vats;
    }

    // koef vt - 82,463372064

    if( esp_log_timestamp() < 10000 ) {
        meter_data.Wt_h = 0.00001;
        meter_data.P = 0.0002622;
        meter_data.A = 0.000467106;
        meter_data.U = 0.036085469;
    }
    else {
        meter_data.Wt_h = 0.0192;
        meter_data.Wt_h = (double)( ( meter_data.Wt_h * wt / 1000000 ) + whats );
    }
    whats = meter_data.Wt_h;
    old_vats = dan;

    meter_data.KWt_h = (float)( meter_data.Wt_h / 1000 );

    return dan;
}

void Meter_cc::get_meter_data( void* arg ) {
    static uint64_t delay_tm = ( ( esp_timer_get_time() / 1000000 ) + 1 ) * 1000000;
    delay_tm = delay_tm - esp_timer_get_time();
    per_state = IDL;
    while( per_state != 0xfe ) {
        vTaskDelay( pdMS_TO_TICKS( delay_tm / 1000 ) );

        get_meter();
#ifdef DEBUG_M
        printf( "\r\n[%d] U=%f, A=%f, P=%f, W_h=%f\r\n",
                esp_log_timestamp(),
                meter_data.U,
                meter_data.A,
                meter_data.P,
                meter_data.Wt_h );
#endif
        if( meter_data.U == 0 )
            init_stpm();

        uint64_t next_tm = ( ( esp_timer_get_time() / 1000000 ) * 1000000 ) + 1000000;
        if( next_tm < esp_timer_get_time() )
            next_tm = next_tm + 500000;
        delay_tm = next_tm - esp_timer_get_time();
        if( arg != NULL )
            break;
    }
}

uint32_t Meter_cc::rx_task2() {
    static uint8_t data1[124];
    uint32_t dan = 0;
    uint8_t cou = 0;
    int read_u = 0;
    size_t len_u;
    while( cou < 2 ) {
        read_u = uart_read_bytes( U_WORK, &data1[read_u], 5, 100 );
        uart_get_buffered_data_len( U_WORK, &len_u );
        if( len_u ) {
            read_u = read_u + uart_read_bytes( U_WORK, &data1[read_u - 1], len_u, 0 );
        }
        // printf("READ %d b\r\n",read_u);
        if( read_u > 4 ) {
            uint8_t* data;
            data = &data1[read_u - 5];
            uint8_t crc = CRC8( data );
            if( crc != data[4] ) {
                if( crc != revers( data[4] ) ) {
                    printf( "\r\nCRC ERROR\r\n" );
                    return dan;
                }
            }
#ifdef DEBUG_M
            printf( "\r\nDATA from stpm DATA: 0x%02X 0x%02X 0x%02X 0x%02X\r\n", data[3], data[2], data[1], data[0] );
#endif
            dan = ( data[3] << 24 ) + ( data[2] << 16 ) + ( data[1] << 8 ) + data[0];
            break;
        }
        else {
            cou++;
        }
    }
    return dan;
}

#endif

Meter_cc::Meter_cc() {}

Meter_cc& Meter_cc::get() {
    static Meter_cc meter;
    return meter;
}

void Meter_cc::Meter_reset() {
    meter_data.KWt_h = 0;
    meter_data.Wt_h = 0;
#ifdef CONFIG_METER_CHIP_STPM
    /*
    sendData2( DSP_CR3, IDL, 0, 0 );  // STOP S/W_AUTO_Latch LED_OFF
    vTaskDelay( pdMS_TO_TICKS( 20 ) );
    sendData2( DSP_CR3, DSP_CR3 + 1, 0x00, 0x03 );  // STOP S/W_AUTO_Latch LED_OFF
    vTaskDelay( pdMS_TO_TICKS( 200 ) );
    sendData2( DSP_CR3, DSP_CR3 + 1, 0x10, 0x03 );  // STOP S/W_AUTO_Latch LED_OFF
    vTaskDelay( pdMS_TO_TICKS( 100 ) );
    sendData2( DSP_CR3, IDL, 0, 0 );  // STOP S/W_AUTO_Latch LED_OFF
    init_stpm();
    */

    whats = 0;
#endif
}

void Meter_cc::Meter_set_consumed_energy_meter( float KWt_h ) {
    meter_data.KWt_h += KWt_h;
    meter_data.Wt_h += KWt_h * 1000;
}

#endif
