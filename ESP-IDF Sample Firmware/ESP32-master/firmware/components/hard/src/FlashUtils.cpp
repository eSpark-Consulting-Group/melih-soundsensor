#include "FlashUtils.h"
#include "esp_spi_flash.h"

namespace hard {

bool eraseFlash( std::size_t start, std::size_t size ) {
    return ESP_OK == spi_flash_erase_range( start, size );
}

bool writeToFlash( std::size_t dest, const void* src, std::size_t size ) {
    return ESP_OK == spi_flash_write( dest, src, size );
}

}  // namespace hard
