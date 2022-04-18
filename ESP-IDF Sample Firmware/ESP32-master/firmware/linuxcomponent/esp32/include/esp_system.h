#include "esp_err.h"

/**
 * @brief Chip models
 */
typedef enum {
    CHIP_ESP32 = 1,  //!< ESP32
} esp_chip_model_t;

/* Chip feature flags, used in esp_chip_info_t */
#define CHIP_FEATURE_EMB_FLASH ( 1UL << ( 0 ) )  //!< Chip has embedded flash memory
#define CHIP_FEATURE_WIFI_BGN ( 1UL << ( 1 ) )   //!< Chip has 2.4GHz WiFi
#define CHIP_FEATURE_BLE ( 1UL << ( 4 ) )        //!< Chip has Bluetooth LE
#define CHIP_FEATURE_BT ( 1UL << ( 5 ) )         //!< Chip has Bluetooth Classic

/**
 * @brief The structure represents information about the chip
 */
typedef struct {
    esp_chip_model_t model;  //!< chip model, one of esp_chip_model_t
    uint32_t features;       //!< bit mask of CHIP_FEATURE_x feature flags
    uint8_t cores;           //!< number of CPU cores
    uint8_t revision;        //!< chip revision number
} esp_chip_info_t;

/**
 * @brief  Restart PRO and APP CPUs.
 *
 * This function can be called both from PRO and APP CPUs.
 * After successful restart, CPU reset reason will be SW_CPU_RESET.
 * Peripherals (except for WiFi, BT, UART0, SPI1, and legacy timers) are not reset.
 * This function does not return.
 */
void esp_restart( void ) __attribute__( ( noreturn ) );

/**
 * @brief  Return base MAC address which is factory-programmed by Espressif in BLK0 of EFUSE.
 *
 * @param  mac  base MAC address, length: 6 bytes.
 *
 * @return ESP_OK on success
 */
esp_err_t esp_efuse_mac_get_default( uint8_t* mac );

/**
 * @brief Fill an esp_chip_info_t structure with information about the chip
 * @param[out] out_info structure to be filled
 */
void esp_chip_info( esp_chip_info_t* out_info );

/**
 * Get IDF version
 *
 * @return constant string from IDF_VER
 */
const char* esp_get_idf_version( void );
