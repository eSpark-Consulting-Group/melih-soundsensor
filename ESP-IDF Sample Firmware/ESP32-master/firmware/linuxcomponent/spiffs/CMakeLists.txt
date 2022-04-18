add_library(spiffs STATIC
	src/esp_spiffs.c
	)

target_include_directories(spiffs
	PUBLIC include
	)

target_link_libraries(spiffs esp32)
