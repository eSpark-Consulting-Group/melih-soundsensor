file(GLOB_RECURSE NMACL_SOURCES "src/*.cpp")
	
idf_component_register(SRCS ${NMACL_SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES common gasonpp storage)
include(../config_flags_throw.cmake)
