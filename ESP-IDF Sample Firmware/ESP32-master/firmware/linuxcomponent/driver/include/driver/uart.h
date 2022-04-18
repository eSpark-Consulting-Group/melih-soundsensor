#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define UART_FIFO_LEN 512

typedef int32_t esp_err_t;

/* Definitions for error constants. */
#define ESP_OK 0    /*!< esp_err_t value indicating success (no error) */
#define ESP_FAIL -1 /*!< Generic esp_err_t code indicating failure */

#define UART_PIN_NO_CHANGE \
    ( -1 ) /*!< Constant for uart_set_pin function which indicates that UART pin should not be changed */

/**
 * @brief UART peripheral number
 */
typedef enum {
    UART_NUM_0 = 0x0,
    UART_NUM_1 = 0x1,
    UART_NUM_2 = 0x2,
    UART_NUM_MAX,
} uart_port_t;

/**
 * @brief UART word length constants
 */
typedef enum {
    UART_DATA_5_BITS = 0x0, /*!< word length: 5bits*/
    UART_DATA_6_BITS = 0x1, /*!< word length: 6bits*/
    UART_DATA_7_BITS = 0x2, /*!< word length: 7bits*/
    UART_DATA_8_BITS = 0x3, /*!< word length: 8bits*/
    UART_DATA_BITS_MAX = 0x4,
} uart_word_length_t;

/**
 * @brief UART parity constants
 */
typedef enum {
    UART_PARITY_DISABLE = 0x0, /*!< Disable UART parity*/
    UART_PARITY_EVEN = 0x2,    /*!< Enable UART even parity*/
    UART_PARITY_ODD = 0x3      /*!< Enable UART odd parity*/
} uart_parity_t;

/**
 * @brief UART stop bits number
 */
typedef enum {
    UART_STOP_BITS_1 = 0x1,   /*!< stop bit: 1bit*/
    UART_STOP_BITS_1_5 = 0x2, /*!< stop bit: 1.5bits*/
    UART_STOP_BITS_2 = 0x3,   /*!< stop bit: 2bits*/
    UART_STOP_BITS_MAX = 0x4,
} uart_stop_bits_t;

/**
 * @brief UART hardware flow control modes
 */
typedef enum {
    UART_HW_FLOWCTRL_DISABLE = 0x0, /*!< disable hardware flow control*/
    UART_HW_FLOWCTRL_RTS = 0x1,     /*!< enable RX hardware flow control (rts)*/
    UART_HW_FLOWCTRL_CTS = 0x2,     /*!< enable TX hardware flow control (cts)*/
    UART_HW_FLOWCTRL_CTS_RTS = 0x3, /*!< enable hardware flow control*/
    UART_HW_FLOWCTRL_MAX = 0x4,
} uart_hw_flowcontrol_t;

/**
 * @brief UART configuration parameters for uart_param_config function
 */
typedef struct {
    int baud_rate;                   /*!< UART baud rate*/
    uart_word_length_t data_bits;    /*!< UART byte size*/
    uart_parity_t parity;            /*!< UART parity mode*/
    uart_stop_bits_t stop_bits;      /*!< UART stop bits*/
    uart_hw_flowcontrol_t flow_ctrl; /*!< UART HW flow control mode (cts/rts)*/
    uint8_t rx_flow_ctrl_thresh;     /*!< UART HW RTS threshold*/
    bool use_ref_tick;               /*!< Set to true if UART should be clocked from REF_TICK */
} uart_config_t;

int uart_set_dev_path( uart_port_t uart_num, const char* tty );
esp_err_t uart_driver_install( uart_port_t uart_num,
                               int rx_buffer_size,
                               int tx_buffer_size,
                               int queue_size,
                               QueueHandle_t* uart_queue,
                               int intr_alloc_flags );
esp_err_t uart_param_config( uart_port_t uart_num, const uart_config_t* uart_config );
esp_err_t uart_set_pin( uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num );
int uart_write_bytes( uart_port_t uart_num, const char* src, size_t size );
int uart_read_bytes( uart_port_t uart_num, uint8_t* buf, uint32_t length, TickType_t ticks_to_wait );
esp_err_t uart_get_buffered_data_len( uart_port_t uart_num, size_t* size );
