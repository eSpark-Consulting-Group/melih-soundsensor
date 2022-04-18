add_library(log STATIC
	log.c
	)

target_include_directories(log
	PUBLIC	include
	)

target_link_libraries(log freertos)
