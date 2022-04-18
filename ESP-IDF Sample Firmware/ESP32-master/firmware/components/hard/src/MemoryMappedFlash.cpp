#include <MemoryMappedFlash.h>
#include <cstdint>
#include <esp_partition.h>
#include "DebugUtils.h"

namespace hard {

bool MemoryMappedFlash::map( std::size_t source_addr, std::size_t size ) {
    unmap();
    m_mmap_handle = std::shared_ptr<spi_flash_mmap_handle_t>(
        new spi_flash_mmap_handle_t, []( spi_flash_mmap_handle_t* handle ) { spi_flash_munmap( *handle ); } );
    if( ESP_OK != spi_flash_mmap( source_addr, size, SPI_FLASH_MMAP_DATA, &m_pointer, &( *m_mmap_handle ) ) ) {
        m_mmap_handle.reset();
        return false;
    }
    m_size = size;
    m_flash_offset = source_addr;
    return true;
}

bool MemoryMappedFlash::map_partition( const char* name, int type ) {
    unmap();
    const esp_partition_t* partition =
        esp_partition_find_first( static_cast<esp_partition_type_t>( type ), ESP_PARTITION_SUBTYPE_ANY, name );
    if( !partition ) {
        FUNC_LOGE( "Can't find %s partition", name );
        return false;
    }
    m_mmap_handle = std::shared_ptr<spi_flash_mmap_handle_t>(
        new spi_flash_mmap_handle_t, []( spi_flash_mmap_handle_t* handle ) { spi_flash_munmap( *handle ); } );
    if( ESP_OK !=
        esp_partition_mmap( partition, 0, partition->size, SPI_FLASH_MMAP_DATA, &m_pointer, &( *m_mmap_handle ) ) ) {
        m_mmap_handle.reset();
        return false;
    }
    m_size = partition->size;
    m_flash_offset = partition->address;
    return true;
}

void MemoryMappedFlash::unmap() {
    m_mmap_handle.reset();
    m_pointer = nullptr;
    m_flash_offset = 0;
}

const void* MemoryMappedFlash::get() const {
    return m_pointer;
}

std::size_t MemoryMappedFlash::get_flash_offset() const {
    return m_flash_offset;
}

std::size_t MemoryMappedFlash::size() const {
    return m_size;
}

}  // namespace hard
