file(GLOB_RECURSE SOURCES "src/*.cpp")

idf_component_register( SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        PRIV_INCLUDE_DIRS "priv_include"
                        REQUIRES bus common jssp devices_discovery )

include(../config_flags_throw.cmake)
