add_library(main STATIC
	src/factory_test_main.cpp
	)

target_include_directories(main
	PUBLIC include
	)

target_link_libraries(main common hard)
