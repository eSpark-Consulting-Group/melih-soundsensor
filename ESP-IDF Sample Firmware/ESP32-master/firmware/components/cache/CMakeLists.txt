file(GLOB CACHE_SRC src/*.cpp)

idf_component_register(SRCS ${CACHE_SRC}
                       INCLUDE_DIRS "include"
                       REQUIRES common)
include(../config_flags_throw.cmake)