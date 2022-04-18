add_library(lwip STATIC
	src/apps/sntp.c
	)

target_include_directories(lwip
	PUBLIC include
	)
