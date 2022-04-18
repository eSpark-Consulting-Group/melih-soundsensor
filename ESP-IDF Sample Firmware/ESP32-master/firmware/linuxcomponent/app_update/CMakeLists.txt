add_library(app_update STATIC
	src/esp_ota_ops.c
	)

target_include_directories(app_update
	PUBLIC	include
	)
