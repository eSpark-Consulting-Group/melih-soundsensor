add_library(esp32 STATIC
	esp_system.c
	esp_timer.c
	)

target_include_directories(esp32
	PUBLIC	include
	)

target_link_libraries(esp32 nvs_flash)
