add_library(nvs_flash STATIC
	src/nvs.c
	)

target_include_directories(nvs_flash
	PUBLIC include
	)

target_link_libraries(nvs_flash esp32)
