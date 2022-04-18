#add_library(plug_hub STATIC
#    src/PlugHub.cpp
#    src/PlugHubGateway.cpp
#    src/PlugHubManager.cpp
#	)

#target_include_directories(plug_hub
#	PUBLIC include
#	)

#target_link_libraries(plug_hub common)
set(PLUGHUB_SRC
	src/PlugHub.cpp
	src/PlugHubGateway.cpp
	src/PlugHubManager.cpp)

idf_component_register(SRCS ${PLUGHUB_SRC}
                       INCLUDE_DIRS "include"
                       REQUIRES common)
