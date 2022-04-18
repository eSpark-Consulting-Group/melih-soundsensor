add_library(heap STATIC
	esp_heap_caps.c
	)

target_include_directories(heap
	PUBLIC	include
	)
