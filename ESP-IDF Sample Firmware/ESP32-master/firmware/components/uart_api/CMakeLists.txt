if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "uart")

    file(GLOB_RECURSE COMPONENT_SOURCES "src/*.cpp")
	
    idf_component_register(SRCS ${COMPONENT_SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES network common gasonpp nma_client)
    include(../config_flags_throw.cmake)

endif()