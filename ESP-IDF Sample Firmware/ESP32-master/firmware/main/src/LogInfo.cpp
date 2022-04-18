#include "DebugUtils.h"
#include "hub_core/Common.h"
#include "FirmwareVersion.h"

#include <esp_system.h>
#include <esp_spi_flash.h>

void logInfo() {
    FUNC_LOGE( "\n--FIRMWARE VERSION INFO --\nModel: %s\nArchitecture: %s\nFirmware: %s\nKernel: %s\nHardware: "
               "%s\nSerial: %s\nBuild time: "
               "%s\nBuilder: %s\nBranch: %s\nCommit: %s\n",
               MODEL,
               HARDWARE_ARCH,
               FIRMWARE_VERSION,
               KERNEL_VERSION,
               HARDWARE_VERSION,
               core::serial(),
               BUILD_TIME,
               BUILD_BUILDER,
               BUILD_BRANCH,
               BUILD_COMMIT );

    esp_chip_info_t chip_info;
    esp_chip_info( &chip_info );
    FUNC_LOGI( "This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
               chip_info.cores,
               ( chip_info.features & CHIP_FEATURE_BT ) ? "/BT" : "",
               ( chip_info.features & CHIP_FEATURE_BLE ) ? "/BLE" : "" );

    FUNC_LOGI( "silicon revision %d, ", chip_info.revision );

    FUNC_LOGI( "%zuMB %s flash",
               spi_flash_get_chip_size() / ( 1024 * 1024 ),
               ( chip_info.features & CHIP_FEATURE_EMB_FLASH ) ? "embedded" : "external" );

    FUNC_LOGI( "ESP version: %s", esp_get_idf_version() );
    FUNC_LOGI( "Serial: %s", core::serial() );
}
