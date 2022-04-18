file(GLOB_RECURSE SOURCES "src/*.cpp")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS include include/other_commands 
                       REQUIRES common littlefs esp32 soc script)

include(../config_flags_throw.cmake)
