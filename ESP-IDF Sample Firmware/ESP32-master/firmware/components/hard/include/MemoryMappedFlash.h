#pragma once
#include <type_traits>
#include <memory>
#include <esp_spi_flash.h>
#include <esp_partition.h>

namespace hard {

class MemoryMappedFlash {
public:
    static constexpr auto USER_TYPE = 0x40;
    static constexpr auto DATA_TYPE = ESP_PARTITION_TYPE_DATA;
    bool map( std::size_t source_addr, std::size_t size );
    bool map_partition( const char* name, const int type = USER_TYPE );
    void unmap();
    const void* get() const;
    std::size_t get_flash_offset() const;
    std::size_t size() const;

private:
    const void* m_pointer;
    std::size_t m_flash_offset;
    std::size_t m_size;
    std::shared_ptr<spi_flash_mmap_handle_t> m_mmap_handle;
};

}  // namespace hard
