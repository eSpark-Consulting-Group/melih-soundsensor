add_library(freertos STATIC
	croutine.c
	event_groups.c
	list.c
	queue.c
	tasks.c
	timers.c
    linuxport/port.c
    linuxport/heap_3.c
	)

target_include_directories(freertos
	PUBLIC	include
			linuxport
	PRIVATE include/freertos
	)

#target_link_libraries(freertos common)
