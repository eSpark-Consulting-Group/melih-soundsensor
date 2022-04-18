#add_library(miniz STATIC
#	src/miniz.c
#	)

#target_include_directories(miniz
#	PUBLIC src
#	)

idf_component_register(SRCS "src/miniz.c"
                       INCLUDE_DIRS "src"
                       REQUIRES )
include(../config_flags_throw.cmake)