#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <driver/uart.h>

void app_main( void );

int main( int argc, char* argv[] ) {
    assert( argc > 1 );
    uart_port_t uart_port = UART_NUM_0;
    int retCode = uart_set_dev_path( uart_port, argv[1] );  // TODO
    if( retCode != 0 ) {
        return retCode;
    }
    uart_driver_install( uart_port, 0, 0, 0, NULL, 0 );
    if( retCode != 0 ) {
        return retCode;
    }

    for( int i = 0; i < 10; i++ ) {
        uint8_t uarttxbff[UART_FIFO_LEN];
        for( size_t i = 0; i < sizeof( uarttxbff ); i++ ) {
            uarttxbff[i] = rand();
        }
        int writeLen = uart_write_bytes( uart_port, uarttxbff, sizeof( uarttxbff ) );
        printf( "Write %d bytes | ", writeLen );

        int delay_us = 100000;
        usleep( delay_us );

        uint8_t uartrxbff[UART_FIFO_LEN];
        int readLen = uart_read_bytes( uart_port, uartrxbff, sizeof( uartrxbff ), 0 );
        printf( "Read %d bytes | ", readLen );

        printf( "Check: " );
        if( memcmp( uarttxbff, uartrxbff, sizeof( uartrxbff ) ) ) {
            printf( "error\n" );
            return 1;
        }

        printf( "ok\n" );
    }

    //    app_main();
    return 0;
}
