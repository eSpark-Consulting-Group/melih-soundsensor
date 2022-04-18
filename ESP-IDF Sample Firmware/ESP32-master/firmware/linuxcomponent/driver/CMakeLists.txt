add_library(driver STATIC
	src/uart.c
	)

target_include_directories(driver
	PUBLIC include
	PRIVATE include/driver
	)

target_link_libraries(driver freertos)
