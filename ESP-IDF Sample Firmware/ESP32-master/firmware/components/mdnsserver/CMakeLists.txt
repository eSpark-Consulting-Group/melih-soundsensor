#add_library(mdnsserver STATIC
#	src/MDnsServer.cpp
#	)
#
#target_include_directories(network
#	PUBLIC include
#	)
#
#target_link_libraries(mdnsserver mdns common)
idf_component_register(SRCS "src/MDnsServer.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES mdns common)
include(../config_flags_throw.cmake)