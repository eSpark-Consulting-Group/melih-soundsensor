file( GLOB_RECURSE NMACL_SOURCES "src/*.cpp" )
	
idf_component_register( SRCS ${NMACL_SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES zwave network common gasonpp jssp cloud wifi_service2 
                                 favorite_manager time_service devices_discovery )

include(../config_flags_throw.cmake)
