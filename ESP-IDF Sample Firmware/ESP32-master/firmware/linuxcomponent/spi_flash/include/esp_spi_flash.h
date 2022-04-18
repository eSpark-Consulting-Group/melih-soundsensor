#include <stddef.h>

/**
 * @brief  Get flash chip size, as set in binary image header
 *
 * @note This value does not necessarily match real flash size.
 *
 * @return size of flash chip, in bytes
 */
size_t spi_flash_get_chip_size();
