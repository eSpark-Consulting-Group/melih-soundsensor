file( GLOB_RECURSE SOURCES "src/*.cpp" )

idf_component_register( SRCS ${SOURCES}
					    INCLUDE_DIRS include
                        REQUIRES common script scriptholder app_update soc local_nma_srv 
                                 local_tcp_srv uart_api cloud_nma_client wifi_service2 
                                 time_service devices_discovery )


