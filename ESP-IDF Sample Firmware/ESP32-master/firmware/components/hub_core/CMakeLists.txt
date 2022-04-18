file(GLOB_RECURSE SOURCES "src/*.cpp")

set(depends
	app_update 
	znet 
	bus 
	common 
	model 
	lwip 
	driver 
	scenes
	cloud
	cloud_storage
	wifi_service2
)

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_hub")
	list(APPEND depends plug_hub)
endif()	

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_2hub")
	list(APPEND depends plug_hub)
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_3hub")
	list(APPEND depends plug_hub)
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "water_valve")
	list(APPEND depends water_valve)
endif()

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES ${depends}
                       )
include(../config_flags_throw.cmake)
