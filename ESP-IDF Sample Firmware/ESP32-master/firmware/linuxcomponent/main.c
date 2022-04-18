#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <driver/uart.h>

void app_main( void );

int main( int argc, char *argv[] )
{
    assert( argc > 1 );
    uart_port_t uart_port = UART_NUM_0;
    int retCode = uart_set_dev_path(uart_port, argv[1]); //TODO
    if( retCode != 0){
        return retCode;
    }

    app_main();
    return 0;
}
