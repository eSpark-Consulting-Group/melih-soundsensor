#TODO: We shouldn't compile wifi stuff for uart api
#if(NOT $ENV{DEPLOY_BOARD_VERSION} STREQUAL "uart")

    file(GLOB_RECURSE SOURCES "src/*.cpp")

    idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS include
                        PRIV_INCLUDE_DIRS priv_include
                        REQUIRES log esp_wifi wifi_provisioning tcpip_adapter bus hub_core system )

    include(../config_flags_throw.cmake)

#endif()	