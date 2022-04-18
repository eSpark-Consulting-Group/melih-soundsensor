add_library(spi_flash STATIC
	src/esp_spi_flash.c
	)

target_include_directories(spi_flash
	PUBLIC include
	)

