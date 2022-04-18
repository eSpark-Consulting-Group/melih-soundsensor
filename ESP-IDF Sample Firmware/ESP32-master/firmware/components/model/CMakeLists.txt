file(GLOB_RECURSE SOURCES "src/*.cpp")

set(link_libraries common cache storage nma_client cloud)
if(DEPLOY_BOARD_VERSION STREQUAL "plug_hub")
list(APPEND link_libraries plug_hub)
endif()
if(DEPLOY_BOARD_VERSION STREQUAL "plug_2hub")
list(APPEND link_libraries plug_hub)
endif()
if(DEPLOY_BOARD_VERSION STREQUAL "plug_3hub")
    list(APPEND link_libraries plug_hub)
endif()
if(DEPLOY_BOARD_VERSION STREQUAL "water_valve")
list(APPEND link_libraries water_valve)
endif()
if(DEPLOY_BOARD_VERSION STREQUAL "lamp_rev")
list(APPEND link_libraries lamp)
endif()
#target_link_libraries(model ${link_libraries})

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES ${link_libraries})
include(../config_flags_throw.cmake)