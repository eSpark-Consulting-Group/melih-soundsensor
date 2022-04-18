#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <linux/limits.h>
#include "uart.h"

typedef struct {
    int ftty;
    struct termios tio, btio;
    char tty[PATH_MAX];
} ttyData_t;

static ttyData_t ttyData[UART_NUM_MAX] = { [0 ... UART_NUM_MAX - 1].ftty = -1 };

int uart_set_dev_path( uart_port_t uart_num, const char* tty ) {
    assert( uart_num < UART_NUM_MAX );
    strncpy( ttyData[uart_num].tty, tty, sizeof( ttyData[uart_num].tty ) );
    return 0;
}

esp_err_t uart_driver_install( uart_port_t uart_num,
                               int rx_buffer_size,
                               int tx_buffer_size,
                               int queue_size,
                               QueueHandle_t* uart_queue,
                               int intr_alloc_flags ) {
    (void)rx_buffer_size;
    (void)tx_buffer_size;
    (void)queue_size;
    (void)uart_queue;
    (void)intr_alloc_flags;
    assert( uart_num < UART_NUM_MAX );
    printf( "open %s\n", ttyData[uart_num].tty );

    /// INFO: open tty better in non block mode
    ttyData[uart_num].ftty = open( ttyData[uart_num].tty, O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK );
    if( ttyData[uart_num].ftty < 0 ) {
        fprintf( stderr, "Open TTY failed [err: %d]\n", errno );
        return ESP_FAIL;
    }
    if( fcntl( ttyData[uart_num].ftty, F_SETFL, fcntl( ttyData[uart_num].ftty, F_GETFL ) & ~O_NONBLOCK ) < 0 ) {
        fprintf( stderr, "Set TTY to block mode failed [err: %d]\n", errno );
        close( ttyData[uart_num].ftty );
        ttyData[uart_num].ftty = -1;
        return ESP_FAIL;
    }
    if( !isatty( ttyData[uart_num].ftty ) ) {
        fprintf( stderr, "Is not a TTY [err: %d]\n", errno );
        close( ttyData[uart_num].ftty );
        ttyData[uart_num].ftty = -1;
        return ESP_FAIL;
    }
    if( tcgetattr( ttyData[uart_num].ftty, &ttyData[uart_num].tio ) < 0 ) {
        fprintf( stderr, "Get TTY config failed\n" );
        close( ttyData[uart_num].ftty );
        ttyData[uart_num].ftty = -1;
        return ESP_FAIL;
    }
    memcpy( &ttyData[uart_num].btio, &ttyData[uart_num].tio,
            sizeof( struct termios ) );  // backup current TTY config

    // 115200 8N1
    cfmakeraw( &ttyData[uart_num].tio );
    if( cfsetspeed( &ttyData[uart_num].tio, B115200 ) < 0 ) {
        fprintf( stderr, "Set TTY baud rate failed\n" );
        close( ttyData[uart_num].ftty );
        ttyData[uart_num].ftty = -1;
        return ESP_FAIL;
    }
    ttyData[uart_num].tio.c_cflag &= ~CSIZE;
    ttyData[uart_num].tio.c_cflag |= CS8;
    ttyData[uart_num].tio.c_cflag &= ~( PARENB | PARODD );
    ttyData[uart_num].tio.c_cflag |= 0;
    ttyData[uart_num].tio.c_cflag &= ~CSTOPB;
    ttyData[uart_num].tio.c_cflag |= 0;
    ttyData[uart_num].tio.c_cflag &= ~CRTSCTS;
    ttyData[uart_num].tio.c_cflag |= 0;
    ttyData[uart_num].tio.c_cc[VMIN] = 0;
    ttyData[uart_num].tio.c_cc[VTIME] = 0;
    if( tcsetattr( ttyData[uart_num].ftty, TCSAFLUSH, &ttyData[uart_num].tio ) < 0 ) {
        fprintf( stderr, "Set TTY attrs failed\n" );
        close( ttyData[uart_num].ftty );
        ttyData[uart_num].ftty = -1;
        return EXIT_FAILURE;
    }
    return ESP_OK;
}

esp_err_t uart_param_config( uart_port_t uart_num, const uart_config_t* uart_config ) {
    return ESP_OK;
}

esp_err_t uart_set_pin( uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num ) {
    return ESP_OK;
}

int uart_write_bytes( uart_port_t uart_num, const char* src, size_t size ) {
    assert( uart_num < UART_NUM_MAX );
    assert( src != NULL );

    if( ttyData[uart_num].ftty < 0 ) {
        fprintf( stderr, "uart write: tty not opened!\n" );
        return -1;
    }

    return write( ttyData[uart_num].ftty, (const char*)src, size );
}

int uart_read_bytes( uart_port_t uart_num, uint8_t* buf, uint32_t length, TickType_t ticks_to_wait ) {
    assert( uart_num < UART_NUM_MAX );
    assert( buf != NULL );

    if( ttyData[uart_num].ftty < 0 ) {
        fprintf( stderr, "uart read: tty not opened!\n" );
        return -1;
    }

    return read( ttyData[uart_num].ftty, buf, length );

    return 0;
}

esp_err_t uart_get_buffered_data_len( uart_port_t uart_num, size_t* size ) {
    assert( uart_num < UART_NUM_MAX );
    assert( size != NULL );
#warning
    return ESP_OK;
}
