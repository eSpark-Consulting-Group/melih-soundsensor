#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"
#include "sdkconfig.h"
#include "znet/znet.h"
#include "LedDevice.h"
#include "ResetZW.h"
#include "sdkconfig.h"

#ifdef CONFIG_BOARD_TYPE_WATER_VALVE
static constexpr auto LED_OPEN = (gpio_num_t)CONFIG_IO_LED_OPEN;
static constexpr auto LED_CLOSE = (gpio_num_t)CONFIG_IO_LED_CLOSE;
static constexpr auto BUTTON_OPEN = (gpio_num_t)CONFIG_IO_BUTTON_OPEN;
static constexpr auto BUTTON_CLOSE = (gpio_num_t)CONFIG_IO_BUTTON_CLOSE;
static constexpr auto HALL_OPEN = (gpio_num_t)CONFIG_IO_HALL_OPEN;
static constexpr auto HALL_CLOSE = (gpio_num_t)CONFIG_IO_HALL_CLOSE;
#endif

static constexpr uint16_t InputGPIO = CONFIG_IO_BUTTON;

////////// BASE //////////

static void initialize_nvs() {
    esp_err_t err = nvs_flash_init();
    if( err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}

static void initialize_console() {
    /* Disable buffering on stdin and stdout */
    setvbuf( stdin, NULL, _IONBF, 0 );
    setvbuf( stdout, NULL, _IONBF, 0 );

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings( ESP_LINE_ENDINGS_CR );
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings( ESP_LINE_ENDINGS_CRLF );

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    uart_config_t uart_config = {};
    uart_config.baud_rate = CONFIG_CONSOLE_UART_BAUDRATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.use_ref_tick = true;
    ESP_ERROR_CHECK( uart_param_config( (uart_port_t)CONFIG_CONSOLE_UART_NUM, &uart_config ) );

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install( (uart_port_t)CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0 ) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver( CONFIG_CONSOLE_UART_NUM );

    /* Initialize the console */
    esp_console_config_t console_config = {};
    console_config.max_cmdline_args = 8;
    console_config.max_cmdline_length = 256;
    ESP_ERROR_CHECK( esp_console_init( &console_config ) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine( 1 );

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback( &esp_console_get_completion );
    linenoiseSetHintsCallback( (linenoiseHintsCallback*)&esp_console_get_hint );

    /* Set command history size */
    linenoiseHistorySetMaxLen( 100 );
}

////////// WIFI //////////

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

static esp_err_t event_handler( void* ctx, system_event_t* event ) {
    switch( event->event_id ) {
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits( wifi_event_group, CONNECTED_BIT );
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        xEventGroupClearBits( wifi_event_group, CONNECTED_BIT );
        /// stop
        esp_wifi_stop();
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void wifi_initialise( void ) {
    static bool initialized = false;
    if( initialized ) {
        return;
    }
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init( event_handler, NULL ) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
    ESP_ERROR_CHECK( esp_wifi_set_storage( WIFI_STORAGE_RAM ) );
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_NULL ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    initialized = true;
}

static bool wifi_join( const char* ssid, const char* pass, int timeout_ms ) {
    wifi_config_t wifi_config = { 0 };
    strncpy( (char*)wifi_config.sta.ssid, ssid, sizeof( wifi_config.sta.ssid ) );
    if( pass ) {
        strncpy( (char*)wifi_config.sta.password, pass, sizeof( wifi_config.sta.password ) );
    }

    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_STA, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_connect() );

    int bits = xEventGroupWaitBits( wifi_event_group, CONNECTED_BIT, 1, 1, timeout_ms / portTICK_PERIOD_MS );
    return ( bits & CONNECTED_BIT ) != 0;
}

/** Arguments used by 'join' function */
static struct {
    struct arg_int* timeout;
    struct arg_str* ssid;
    struct arg_str* password;
    struct arg_end* end;
} wifi_connect_args;

static int wifi_connect( int argc, char** argv ) {
    printf( "\n" );
    int nerrors = arg_parse( argc, argv, (void**)&wifi_connect_args );
    if( nerrors != 0 ) {
        arg_print_errors( stderr, wifi_connect_args.end, argv[0] );
        return 1;
    }

    bool connected = wifi_join(
        wifi_connect_args.ssid->sval[0], wifi_connect_args.password->sval[0], wifi_connect_args.timeout->ival[0] );
    if( !connected ) {
        ESP_ERROR_CHECK( esp_wifi_stop() );
        ESP_ERROR_CHECK( esp_wifi_start() );
        // error
        printf( "WIFI_RSSI 0\n" );
        return 1;
    }
    wifi_ap_record_t info;
    memset( &info, 0, sizeof( wifi_ap_record_t ) );
    if( ESP_OK == esp_wifi_sta_get_ap_info( &info ) ) {
        printf( "WIFI_RSSI %" PRIi8 "\n", info.rssi );
    }
    else {
        // error
        printf( "WIFI_RSSI 0\n" );
    }
    ESP_ERROR_CHECK( esp_wifi_stop() );
    ESP_ERROR_CHECK( esp_wifi_start() );
    return 0;
}

static void register_wifi_command() {
    wifi_connect_args.timeout = arg_int0( NULL, "timeout", "<t>", "Connection timeout, ms" );
    wifi_connect_args.timeout->ival[0] = 5000;  // set default value
    wifi_connect_args.ssid = arg_str1( NULL, NULL, "<ssid>", "SSID of AP" );
    wifi_connect_args.password = arg_str0( NULL, NULL, "<pass>", "PSK of AP" );
    wifi_connect_args.end = arg_end( 2 );

    const esp_console_cmd_t wifi_connect_cmd = { .command = "TEST_WIFI",
                                                 .help = "Join WiFi AP as a station",
                                                 .hint = NULL,
                                                 .func = &wifi_connect,
                                                 .argtable = &wifi_connect_args };

    ESP_ERROR_CHECK( esp_console_cmd_register( &wifi_connect_cmd ) );
}

////////// ZWAVE //////////

static size_t store_dat_size = 0;
static void* store_dat = NULL;

#ifndef MAX
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#endif

#define UartBufSize ( MAX( 512, UART_FIFO_LEN ) )

static void zn_log( int lvl, const char* fmt, va_list list, void* arg ) {}

static void* zn_alloc( void* ptr, size_t size, void* arg ) {
    (void)arg;
    if( size != 0 )
        return realloc( ptr, size );

    free( ptr );
    return NULL;
}

static uint64_t zn_clock( void* arg ) {
    return xTaskGetTickCount() * 1000UL / configTICK_RATE_HZ;
}

static int zn_uart_init() {
    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    if( uart_param_config( (uart_port_t)CONFIG_UART_PORT, &uart_config ) != ESP_OK ) {
        return 0;
    }
    if( uart_set_pin(
            (uart_port_t)CONFIG_UART_PORT, CONFIG_IO_TX, CONFIG_IO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ) !=
        ESP_OK ) {
        return 0;
    }
    if( uart_driver_install(
            (uart_port_t)CONFIG_UART_PORT, UartBufSize + UART_FIFO_LEN, UartBufSize + UART_FIFO_LEN, 0, NULL, 0 ) !=
        ESP_OK ) {
        return 0;
    }
    return !0;
}

static int zn_uart_write( const void* data, size_t size, size_t* ret_size, void* arg ) {
    if( !data ) {
        if( !ret_size ) {
            return -1;
        }

        *ret_size = UartBufSize;
        return 0;
    }

    if( !size ) {
        return -1;
    }

    int writeLen = uart_write_bytes( (uart_port_t)CONFIG_UART_PORT, (const char*)data, size );
    if( writeLen < 0 )
        return -1;

    if( ret_size )
        *ret_size = writeLen;

    return 0;
}

static int zn_uart_read( void* data, size_t size, size_t* ret_size, void* arg ) {
    if( !data ) {
        if( !ret_size ) {
            return -1;
        }

        if( uart_get_buffered_data_len( (uart_port_t)CONFIG_UART_PORT, ret_size ) != ESP_OK ) {
            *ret_size = 0;
            return -1;
        }

        return 0;
    }

    if( !size ) {
        return -1;
    }

    int readLen = uart_read_bytes( (uart_port_t)CONFIG_UART_PORT, (uint8_t*)data, size, 0 /*no wait*/ );
    if( readLen < 0 )
        return -1;

    if( ret_size )
        *ret_size = readLen;

    return 0;
}

static int zn_store_save( size_t offset, const void* data, size_t size, void* arg ) {
    if( !data || !size ) {
        return -1;
    }

    {
        // memory
        if( !store_dat || ( offset + size > store_dat_size ) ) {
            void* n = realloc( store_dat, offset + size );
            if( !n ) {
                return -1;
            }

            memset( (uint8_t*)n + store_dat_size, 0, offset + size - store_dat_size );
            store_dat = n;
            store_dat_size = offset + size;
        }

        memcpy( (uint8_t*)store_dat + offset, data, size );
    }

    return 0;
}

static int zn_store_load( size_t offset, void* data, size_t size, void* arg ) {
    if( !data || !size ) {
        return -1;
    }

    {
        // memory
        if( !store_dat || ( offset + size > store_dat_size ) ) {
            memset( data, 0, size );
            return 0;
        }

        memcpy( data, (uint8_t*)store_dat + offset, size );
    }

    return 0;
}

static int zn_store_reset( size_t reserve, void* arg ) {
    {
        // memory
        if( store_dat ) {
            free( store_dat );
            store_dat = NULL;
            store_dat_size = 0;
        }
        /*if( reserve ) {
            store_dat = malloc( reserve );
            if( store_dat ) {
                memset( store_dat, 0, reserve );
                store_dat_size = reserve;
            }
        }*/
    }

    return 0;
}

static bool znet_init_done = false;

static void zn_init_finish( void* arg ) {
    znet_init_done = true;
}

static void zn_private_s2_dsk( znet_error_t err, const char* dsk_key, size_t size ) {
    const size_t S2_DSK_KEY_SIZE = 32u;
    const size_t N_S2_DSK_DIGITS = 8u;
    printf( "ZWAVE_S2_DSK " );
    for( size_t i = 0; i < N_S2_DSK_DIGITS; ++i ) {
        uint16_t value = 0;
        if( ZNET_ERR_OK == err && size == S2_DSK_KEY_SIZE ) {
            memcpy( &value, &dsk_key[i * 2], 2 );
            value = ntohs( value );
        }
        printf( "%5.5hu", value );
        if( i < N_S2_DSK_DIGITS - 1 ) {
            printf( "-" );
        }
    }
    printf( "\n" );
}

static znet_callbacks_t zn_cb = { .arg = NULL,
                                  .alloc = zn_alloc,
                                  .clock = zn_clock,
                                  .log = zn_log,
                                  .uart_write = zn_uart_write,
                                  .uart_read = zn_uart_read,
                                  .uart_reset = NULL,
                                  .store_save = zn_store_save,
                                  .store_load = zn_store_load,
                                  .store_reset = zn_store_reset,
                                  .init_finish = zn_init_finish,
                                  .set_default_started = NULL,
                                  .set_default = NULL,
                                  .network_update = NULL,
                                  .request_neighbor_result = NULL,
                                  .private_s2_dsk = zn_private_s2_dsk };

static int zwave_sendnif( int argc, char** argv ) {
    printf( "\n" );

    static bool uart_inited = false;
    if( !uart_inited ) {
        if( !zn_uart_init() ) {
            // error
            printf( "ZWAVE_HOME_ID 0\n" );
            return 1;
        }
        uart_inited = true;
    }

    static bool zwave_inited = false;
    if( !zwave_inited ) {
        znet_init_done = false;
        if( ZNET_ERR_OK != znet_init( &zn_cb ) ) {
            // error
            printf( "ZWAVE_HOME_ID 0\n" );
            return 1;
        }
        zwave_inited = true;
    }

    const size_t NIF_SEND_COUNT = 10;
    const int LOOP_PERIOD_MS = 20;
    const int NIF_SEND_DELAY_MS = 1000;
    bool nif = false;

    for( size_t i = 0; i < NIF_SEND_COUNT; ++i ) {
        size_t n_ticks = NIF_SEND_DELAY_MS / LOOP_PERIOD_MS;
        bool current_nif = false;
        while( n_ticks-- > 0 ) {
            if( znet_init_done && !current_nif ) {
                nif = true;
                current_nif = true;
                znet_send_node_info();
                znet_get_private_s2_dsk();
                extern uint32_t DEVICE_HOME_ID;
                printf( "ZWAVE_HOME_ID %X\n", DEVICE_HOME_ID );
            }

            znet_proc();
            vTaskDelay( LOOP_PERIOD_MS / portTICK_PERIOD_MS );
        }
    }
    if( !nif ) {
        // error
        printf( "ZWAVE_HOME_ID 0\n" );
    }
    return 0;
}

static void register_zwave_command() {
    const esp_console_cmd_t zwave_sendnif_cmd = {
        .command = "TEST_ZWAVE", .help = "Send NIF over ZWave", .hint = NULL, .func = &zwave_sendnif
    };

    ESP_ERROR_CHECK( esp_console_cmd_register( &zwave_sendnif_cmd ) );
}

////////// BUTTON & LED //////////

static bool button_pressed = false;

static void button_gpio_cb( void* arg ) {
    button_pressed = gpio_get_level( (gpio_num_t)InputGPIO ) == 0;
}

static bool button_init() {
    static int initialized( 0 );
    if( initialized ) {
        return true;
    }

    esp_err_t res = ESP_FAIL;

    gpio_pad_select_gpio( InputGPIO );
    res = gpio_set_direction( (gpio_num_t)InputGPIO, GPIO_MODE_INPUT );
    if( res != ESP_OK ) {
        return false;
    }

    res = gpio_set_pull_mode( (gpio_num_t)InputGPIO, GPIO_PULLUP_ONLY );
    if( res != ESP_OK ) {
        return false;
    }

    res = gpio_set_intr_type( (gpio_num_t)InputGPIO, GPIO_INTR_ANYEDGE );
    if( res != ESP_OK ) {
        return false;
    }

    res = gpio_install_isr_service( ESP_INTR_FLAG_IRAM );
    if( ESP_OK != res ) {
        return false;
    }

    res = gpio_isr_handler_add( (gpio_num_t)InputGPIO, button_gpio_cb, NULL );
    if( res != ESP_OK ) {
        return false;
    }

    res = gpio_intr_enable( (gpio_num_t)InputGPIO );
    if( res != ESP_OK ) {
        return false;
    }

    initialized = 1;

    return true;
}

static int hard_test( int argc, char** argv ) {
    printf( "\n" );

    if( !button_init() ) {
        printf( "Failed to initialize button\n" );
        return -1;
    }

    hard::LedDevice led( CONFIG_IO_LED );
    led.ledOff();

    size_t button_pressed_count = 0;

    constexpr auto WAIT_FOR_BUTTON_PRESS_S = 60;
    constexpr auto MIN_PRESS_DURATION_MS = 1000;
    constexpr auto PERIOD_MS = 10;

    bool start_count{ false };

    constexpr auto BLINK_PERIOD_MS = 100;
    led.setBlink( ~0, BLINK_PERIOD_MS );

    // Wait for 60 s
    for( size_t i = 0; i < WAIT_FOR_BUTTON_PRESS_S / PERIOD_MS * 1000; ++i ) {
        // Button must be depressed at the beginning
        if( !start_count && !button_pressed ) {
            start_count = true;
        }

        if( start_count && button_pressed && ++button_pressed_count >= MIN_PRESS_DURATION_MS / PERIOD_MS ) {
            break;
        }

        vTaskDelay( PERIOD_MS / portTICK_PERIOD_MS );
    }
    led.ledOff();
    if( button_pressed_count >= MIN_PRESS_DURATION_MS / PERIOD_MS ) {
        printf( "BUTTON_OK\n" );
    }
    else {
        printf( "BUTTON_ERROR\n" );
    }

    return 0;
}

static void hard_test_command() {
    const esp_console_cmd_t hard_test_cmd = {
        .command = "TEST_BUTTON", .help = "On LED & check BUTTON", .hint = NULL, .func = &hard_test
    };

    ESP_ERROR_CHECK( esp_console_cmd_register( &hard_test_cmd ) );
}

static int get_mac( int argc, char** argv ) {
    printf( "\n" );
    uint8_t mac[6];
    printf( "MAC_ADDRESS " );
    if( ESP_OK != esp_read_mac( mac, ESP_MAC_WIFI_STA ) ) {
        // 0 Means Error
        printf( "0\n" );
        return 1;
    }
    for( size_t i = 0; i < 6; ++i ) {
        printf( "%02X", mac[i] );
    }
    printf( "\n" );
    return 0;
}

static void register_get_mac_command() {
    const esp_console_cmd_t get_mac_cmd = {
        .command = "GET_MAC", .help = "Get MAC address", .hint = NULL, .func = &get_mac
    };

    ESP_ERROR_CHECK( esp_console_cmd_register( &get_mac_cmd ) );
}

static int create_aes256_key( int argc, char** argv ) {
    printf( "\n" );
    printf( "AES256_KEY " );
    constexpr size_t key_length = 32;  // 256/8 = 32
    uint8_t buffer[key_length];
    esp_fill_random( buffer, key_length );
    for( size_t i = 0; i < key_length; ++i ) {
        printf( "%02X", buffer[i] );
    }
    printf( "\n" );
    return 0;
}

static void register_create_aes256_key_command() {
    const esp_console_cmd_t get_mac_cmd = {
        .command = "CREATE_AES256_KEY", .help = "Create AES256 secure key", .hint = NULL, .func = &create_aes256_key
    };

    ESP_ERROR_CHECK( esp_console_cmd_register( &get_mac_cmd ) );
}

#ifdef CONFIG_BOARD_TYPE_WATER_VALVE

uint8_t gpio_wv = 0;

static void WV_gpio_cb( void* arg )
{
    gpio_num_t gpio_i = (gpio_num_t) arg;
    uint8_t st = gpio_get_level( gpio_i );
    if(!st){
    switch (gpio_i){
        case BUTTON_OPEN:
            gpio_wv = 1;
            break;
        case BUTTON_CLOSE:
            gpio_wv = 2;
            break;
        case HALL_OPEN:
            gpio_wv = 3;
            break;
        case HALL_CLOSE:
            gpio_wv = 4;
            break;
        default :
            gpio_wv = 0;
    }
    }
}


static void inst_out_button( gpio_num_t gpioPin, uint8_t state ) {
    gpio_pad_select_gpio( gpioPin );
    gpio_set_direction( gpioPin, GPIO_MODE_OUTPUT );
    gpio_set_level( gpioPin, state );
}

static void inst_input_button( gpio_num_t gpioPin ) {
    gpio_pad_select_gpio( gpioPin );
    gpio_set_direction( gpioPin, GPIO_MODE_INPUT );
//    gpio_intr_disable( gpioPin );

    res = gpio_set_intr_type( gpioPin, GPIO_INTR_ANYEDGE );
    if( res != ESP_OK ) {
        return false;
    }

    res = gpio_install_isr_service( ESP_INTR_FLAG_IRAM );
    if( ESP_OK != res ) {
        return false;
    }

    res = gpio_isr_handler_add( gpioPin, WV_gpio_cb, (void*) gpioPin);

    return;
}

static void set_led(gpio_num_t gpioPin, uint8_t state)
{
    gpio_set_level( gpioPin, state );
}

static void init_WV_led()
{
    inst_out_button( LED_OPEN, 1 );
    inst_out_button( LED_CLOSE, 1 );

    inst_input_button( BUTTON_OPEN );
    inst_input_button( BUTTON_CLOSE );
    inst_input_button( HALL_OPEN );
    inst_input_button( HALL_CLOSE );
}

static int wwv_test( int argc, char** argv ) {
    gpio_wv = 0;
    set_led( LED_OPEN, 0 );
    set_led( LED_CLOSE, 0 );

    constexpr auto WAIT_FOR_BUTTON_PRESS_S = 20;
    constexpr auto MIN_PRESS_DURATION_MS = 1000;
    constexpr auto PERIOD_MS = 10;

    bool start_count{ false };
    uint16_t button_pressed_count = 0;
    uint8_t err_b = 0;

    // Wait for 20 s
    for( size_t i = 0; i < WAIT_FOR_BUTTON_PRESS_S / PERIOD_MS * 1000; ++i ) {
        // Button must be depressed at the beginning
        if( !start_count && gpio_wv == 1 ) {
            start_count = true;
        }

        if( start_count && gpio_wv == 1 && ++button_pressed_count >= MIN_PRESS_DURATION_MS / PERIOD_MS ) {
            set_led( LED_OPEN, 1 );
            err_b = 1;
            break;
        }

        vTaskDelay( PERIOD_MS / portTICK_PERIOD_MS );
    }
    start_count = false;
    button_pressed_count = 0;
    // Wait for 20 s
    for( size_t i = 0; i < WAIT_FOR_BUTTON_PRESS_S / PERIOD_MS * 1000; ++i ) {
        // Button must be depressed at the beginning
        if( !start_count && gpio_wv == 2 ) {
            start_count = true;
        }

        if( start_count && gpio_wv == 2 && ++button_pressed_count >= MIN_PRESS_DURATION_MS / PERIOD_MS ) {
            set_led( LED_CLOSE, 1 );
            err_b = err_b + 2;
            break;
        }

        vTaskDelay( PERIOD_MS / portTICK_PERIOD_MS );
    }

    switch (err_b){
        case 3:
           printf( "BUTTON_OK\n" );
           break;
        default:
            printf( "BUTTON_ERROR\n" );
    }
    return 0;
}

static int wwv_motor_test( int argc, char** argv ) {
    set_led( LED_CLOSE, 0 );
    vTaskDelay( 2000 / portTICK_PERIOD_MS );
    set_led( LED_CLOSE, 1 );
}


static int wwv_hall_test( int argc, char** argv ) {
    set_led( LED_OPEN, 1 );
    set_led( LED_CLOSE, 1 );

    constexpr auto WAIT_FOR_BUTTON_PRESS_S = 10;

    constexpr auto PERIOD_MS = 10;

    uint8_t err_b = 0;

    // Wait for 20 s
    for( size_t i = 0; i < WAIT_FOR_BUTTON_PRESS_S / PERIOD_MS * 1000; ++i ) {
        // Button must be depressed at the beginning
        if(  gpio_wv == 3 && !(err_b&1)) {
            err_b = err_b +1;
            set_led( LED_OPEN, 0 );
        }
        if(  gpio_wv == 4 && !(err_b&2)) {
            err_b = err_b +2;
            set_led( LED_CLOSE, 0 );
        }
        if(err_b == 3){
            vTaskDelay( 1000 / portTICK_PERIOD_MS );
            printf( "HALL_OK\n" );
            set_led( LED_OPEN, 1 );
            set_led( LED_CLOSE, 1 );
            return 0;
        }


        vTaskDelay( PERIOD_MS / portTICK_PERIOD_MS );
    }
    printf( "HALL_ERROR\n" );
    return 1;
}


static void register_WV_test()
{
    const esp_console_cmd_t wwv_test_cmd = {
        .command = "WV_LED_TEST", .help = "Watervalv led and button test", .hint = NULL, .func = &wwv_test
    };

    ESP_ERROR_CHECK( esp_console_cmd_register( &wwv_test_cmd ) );
}

static void register_WV_motor()
{
    const esp_console_cmd_t wwv_motor_cmd = {
        .command = "WV_MOTOR_TEST", .help = "Watervalv motor test", .hint = NULL, .func = &wwv_motor_test
    };

    ESP_ERROR_CHECK( esp_console_cmd_register( &wwv_motor_cmd ) );
}

static void register_WV_hall()
{
    const esp_console_cmd_t wwv_hall_cmd = {
        .command = "WV_HALL_TEST", .help = "Watervalv hall test", .hint = NULL, .func = &wwv_hall_test
    };

    ESP_ERROR_CHECK( esp_console_cmd_register( &wwv_hall_cmd ) );
}

#endif

////////// MAIN //////////

extern "C" void app_main() {
    // This is needed to detect restart during tests and provisioning
    constexpr auto RESTART_MAGIC_MESSAGE = "ESP32RESTARTMAGICMESSAGE\n";
    printf( RESTART_MAGIC_MESSAGE );

    esp_log_level_set( "*", ESP_LOG_ERROR );
    initialize_nvs();
    initialize_console();
    // Need WiFi for true random number generation
    wifi_initialise();

    // Reset ZWave chip
    auto& resetZW = hard::ResetZW::get();
    resetZW.initialize();
    resetZW.resetZW();

    esp_console_register_help_command();
    register_wifi_command();
    register_zwave_command();
    hard_test_command();
    register_get_mac_command();
    register_create_aes256_key_command();

#ifdef CONFIG_BOARD_TYPE_WATER_VALVE
    init_WV_led();
    register_WV_test();
    register_WV_motor();
    register_WV_hall();
#endif

    linenoiseSetDumbMode( 1 );

    for( ;; ) {
        char* line = linenoise( "#" );
        if( line == NULL ) {
            continue;
        }

        int ret;
        esp_err_t err = esp_console_run( line, &ret );
        if( err == ESP_ERR_NOT_FOUND ) {
            printf( "INVALID COMMAND\n" );
        }
        else if( err == ESP_ERR_INVALID_ARG ) {
            printf( "INVALID ARGUMENTS\n" );
        }

        linenoiseFree( line );
    }
}
