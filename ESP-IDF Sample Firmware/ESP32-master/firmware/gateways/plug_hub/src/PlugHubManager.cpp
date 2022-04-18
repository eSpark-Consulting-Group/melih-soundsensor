#include <PlugHub.h>
#include <PlugHubGateway.h>
#include <PlugHubManager.h>
#include <StorageFile.h>
#include <cstring>

#include <DebugUtils.h>
#include "Meter_cc.h"
#include "driver/i2c.h"
#include "Ezlo.h"
#include "LedControl.h"

namespace gateways {
namespace plug_hub {

bool PlugHubManager::initialize() {
#ifdef LAMP
    i2c_port_t i2c_master_port = I2C_NUM_0;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = GPIO_NUM_13;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = GPIO_NUM_12;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    i2c_param_config( i2c_master_port, &conf );
    esp_err_t res = i2c_driver_install( i2c_master_port, conf.mode, 0, 0, 0 );
    if( res != ESP_OK ) {
        printf( "\r\nINIT I2C MASTER ERROR\r\n" );
    }
    vTaskDelay( pdMS_TO_TICKS( 20 ) );

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start( cmd );
    i2c_master_write_byte( cmd, ( 0x77 << 1 ) | I2C_MASTER_WRITE, 1 /* expect ack */ );
    i2c_master_stop( cmd );

    esp_err_t espRc = i2c_master_cmd_begin( I2C_NUM_0, cmd, 20 / portTICK_PERIOD_MS );

    if( espRc == 0 ) {
        printf( "\r\nLAMP FOUND\r\n" );
    }
    else {
        printf( "\r\nLAMP NOT FOUND\r\n\"" );
    }
    i2c_cmd_link_delete( cmd );
    m_plug_state = true;
#else  // LAMP
    m_plug_state = 0;
    if( storeLoad( 0, &m_plug_state, sizeof( m_plug_state ) ) ) {
        storeReset();
        storeSave( 0, &m_plug_state, sizeof( m_plug_state ) );
    };

#ifndef CONFIG_METER_CHIP_STPM

    esp_err_t res = ESP_FAIL;
    gpio_pad_select_gpio( RELAY_A_PIN );
    res = gpio_set_direction( RELAY_A_PIN, GPIO_MODE_OUTPUT );
    if( res != ESP_OK ) {
        FUNC_LOGE( "gpio_set_direction error" );
        return false;
    }
    res = gpio_set_level( RELAY_A_PIN, m_plug_state ? 1 : 0 );
    if( res != ESP_OK ) {
        FUNC_LOGE( "gpio_set_level error" );
        return false;
    }

#else   // CONFIG_METER_CHIP_STPM
    if( m_plug_state ) {
        gpio_set_level( Meter_cc::RELAY_A_PIN, 1 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
        gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );
    }
    else {
        gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );

        vTaskDelay( pdMS_TO_TICKS( 50 ) );
        gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 1 );
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
        gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );
    }
#endif  // CONFIG_METER_CHIP_STPM
#endif  // LAMP

    hard::LedControl::onPlugSwitchOn( m_plug_state );
    return true;
}

PlugHubManager& PlugHubManager::get() {
    static PlugHubManager plug_hub_manager;
    return plug_hub_manager;
}

bool PlugHubManager::onLoop() {
    PlugHub::get().onLoop();

    const auto now = std::chrono::steady_clock::now();
    const auto elapsedTime = now - m_lastDataUpdateTime;
    if( elapsedTime > UpdateMeterValue ) {
        Meter_cc::Meter_values_t* meter_data = Meter_cc::GetMeter();
        PlugHub::get().onUpdateMeterValue( meter_data->U, meter_data->A, meter_data->P, meter_data->KWt_h );
        m_lastDataUpdateTime = now;
    }
    return true;
}

void PlugHubManager::onUpdatePlugState( const bool value ) {
    FUNC_LOGD( "onUpdateValue:  value: %u", value );
    // I2C send

    esp_err_t res = ESP_OK;
#ifdef LAMP
    uint8_t data_wr[2] = { 0x00, 0x00 };
    data_wr[0] = value ? 3 : 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start( cmd );
    i2c_master_write_byte( cmd, ( 0x77 << 1 ) | I2C_MASTER_WRITE, 1 );
    i2c_master_write_byte( cmd, data_wr[0], 1 );
    i2c_master_stop( cmd );
    res = i2c_master_cmd_begin( I2C_NUM_0, cmd, 20 / portTICK_RATE_MS );
    i2c_cmd_link_delete( cmd );
    printf( "\r\nSEND I2C - value:0x%02x\r\n", data_wr[0] );
    if( res != ESP_OK ) {
        printf( "\r\nSEND ERROR\r\n" );
    }
    else {
        printf( "\r\nSEND OK\r\n" );
    }
#else  // LAMP
#ifndef CONFIG_METER_CHIP_STPM
    res = gpio_set_level( RELAY_A_PIN, value ? 1 : 0 );
#else   // CONFIG_METER_CHIP_STPM
    res = ESP_OK;
    if( value ) {
        gpio_set_level( Meter_cc::RELAY_A_PIN, 1 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
        gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );
    }
    else {
        gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );

        vTaskDelay( pdMS_TO_TICKS( 50 ) );
        gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 1 );
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
        gpio_set_level( Meter_cc::RELAY_A_PIN, 0 );
        gpio_set_level( Meter_cc::RELAY_B_PIN, 0 );
    }
    res = ESP_OK;
#endif  // CONFIG_METER_CHIP_STPM
#endif  // LAMP
    if( res != ESP_OK ) {
        FUNC_LOGE( "gpio_set_level error" );
        return;
    }

    m_plug_state = value;
    hard::LedControl::onPlugSwitchOn( value );

    storeSave( 0, &value, sizeof( value ) );

    PlugHub::get().onUpdatePlugState( value );
}

void PlugHubManager::onPressButton() {
    m_plug_state = !m_plug_state;
    onUpdatePlugState( m_plug_state );
}

int PlugHubManager::storeSave( size_t offset, const void* data, size_t size ) {
    if( !data || !size ) {
        FUNC_LOGD( "store save: invalid args!" );
        return -1;
    }

    storage::File file;
    bool& result = file.linkToTransactionStatus();
    if( !( result = file.open( STORE_PATH, "rb+" ) ) ) {
        FUNC_LOGE( "open store for read fail!" );
        return -1;
    }
    FILE* f = file.get();

    if( fseek( f, offset, SEEK_SET ) != 0 ) {
        if( fseek( f, 0, SEEK_END ) != 0 ) {
            FUNC_LOGE( "seek store for write fail!" );
            result = false;
            return -1;
        }

        long int curr_offset = ftell( f );
        if( curr_offset < 0 || curr_offset >= offset ) {
            FUNC_LOGE( "seek store for write fail!" );
            result = false;
            return -1;
        }

        uint8_t buff[8] = {};
        size_t d = offset - curr_offset;
        while( d ) {
            const size_t m = std::min( d, sizeof( buff ) );
            if( fwrite( buff, 1, m, f ) != m ) {
                break;
            }
            d -= m;
        }
        if( fseek( f, offset, SEEK_SET ) != 0 ) {
            FUNC_LOGE( "seek store for write fail!" );
            result = false;
            return -1;
        }
    }

    if( fwrite( data, size, 1, f ) != 1 ) {
        FUNC_LOGE( "write block to store fail!" );
        result = false;
        return -1;
    }

    return 0;
}

int PlugHubManager::storeLoad( size_t offset, void* data, size_t size ) {
    if( !data || !size ) {
        FUNC_LOGD( "store load: invalid args!" );
        return -1;
    }

    storage::File file;
    if( !file.open( STORE_PATH, "rb" ) ) {
        FUNC_LOGE( "open store for read fail!" );
        return -1;
    }
    FILE* f = file.get();

    if( fseek( f, offset, SEEK_SET ) != 0 && ferror( f ) ) {
        FUNC_LOGE( "seek store for read fail!" );
        return -1;
    }

    if( fread( data, size, 1, f ) != 1 ) {
        if( !ferror( f ) && feof( f ) ) {
            memset( data, 0, size );
            return 0;
        }

        FUNC_LOGE( "read block from store fail!" );
        return -1;
    }
    return 0;
}

int PlugHubManager::storeReset() {
    storage::File file;

    if( !file.open( STORE_PATH, "wb" ) ) {
        FUNC_LOGE( "open store for reset fail!" );
        return -1;
    }
    return 0;
}

}  // namespace plug_hub
}  // namespace gateways
