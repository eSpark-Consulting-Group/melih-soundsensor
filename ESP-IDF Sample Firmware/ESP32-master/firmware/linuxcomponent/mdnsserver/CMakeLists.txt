add_library(mdnsserver STATIC
	src/MDnsServer.cpp
	)

target_include_directories(mdnsserver
	PUBLIC include
	)
