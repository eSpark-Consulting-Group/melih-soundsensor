cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXTRA_COMPONENT_DIRS "configs")
set(CMAKE_DEFINED_TEST "DEFINE IN ROOT CMAKE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized")

option(BUILD_FOR_LINUX	"Build for linux"	OFF)
option(BUILD_FOR_FACTORY_TEST	"Build for factory test"	OFF)

if(HUBPLUGINS_ARCHIVE_FMT)
	message(STATUS "Archive format of hubplugins scripts is set by the cmake variable: HUBPLUGINS_ARCHIVE_FMT=${HUBPLUGINS_ARCHIVE_FMT}")
else()
	if("$ENV{HUBPLUGINS_ARCHIVE_FMT}" STREQUAL "")
		set(HUBPLUGINS_ARCHIVE_FMT "MOLFAR_ZZ" CACHE STRING "Archive format of hubplugins scripts, possible values are \"TAR\", \"TAR_ZZ\", \"MOLFAR_ZZ\"")
		message(STATUS "Archive format of hubplugins scripts is set to the default value: HUBPLUGINS_ARCHIVE_FMT=${HUBPLUGINS_ARCHIVE_FMT}")
	else()
		set(HUBPLUGINS_ARCHIVE_FMT "$ENV{HUBPLUGINS_ARCHIVE_FMT}" CACHE)
		message(STATUS "Archive format of hubplugins scripts is taken from the environment: HUBPLUGINS_ARCHIVE_FMT=${HUBPLUGINS_ARCHIVE_FMT}")
	endif()
endif()

# Needed for config_flags_throw.cmake (storage component)
# MUST be executed before project() statement
set(ENV{HUBPLUGINS_ARCHIVE_FMT} "${HUBPLUGINS_ARCHIVE_FMT}")

# Configure ZNET logs
set(ENV{ZNET_LOG_LEVEL} "ZNET_LOG_DEBUG")

include(cmake/configurecmake.cmake)

# Define board
if(DEFINED BOARD)
	if(${BOARD} STREQUAL "DEVBOARD")
		set(ENV{DEPLOY_BOARD_VERSION} "devboard")
	elseif(${BOARD} STREQUAL "REV1")
		set(ENV{DEPLOY_BOARD_VERSION} "rev1")
	elseif(${BOARD} STREQUAL "REV2")
		set(ENV{DEPLOY_BOARD_VERSION} "rev2")
	elseif(${BOARD} STREQUAL "REV3")
		set(ENV{DEPLOY_BOARD_VERSION} "rev3")
	elseif(${BOARD} STREQUAL "PLUG")
		set(ENV{DEPLOY_BOARD_VERSION} "plug_hub")
	elseif(${BOARD} STREQUAL "PLUG2")
		set(ENV{DEPLOY_BOARD_VERSION} "plug_2hub")
	elseif(${BOARD} STREQUAL "PLUG3")
		set(ENV{DEPLOY_BOARD_VERSION} "plug_3hub")
	elseif(${BOARD} STREQUAL "UART")
		set(ENV{DEPLOY_BOARD_VERSION} "uart")
	elseif(${BOARD} STREQUAL "OLD_REV")
		set(ENV{DEPLOY_BOARD_VERSION} "old_rev")
	elseif(${BOARD} STREQUAL "VALVE")
		set(ENV{DEPLOY_BOARD_VERSION} "water_valve")
	else()
		set(ENV{DEPLOY_BOARD_VERSION} "devboard")
	endif()
else()
	set(ENV{DEPLOY_BOARD_VERSION} "devboard")
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_hub")
		list(APPEND EXTRA_COMPONENT_DIRS "gateways/plug_hub")
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_2hub")
		list(APPEND EXTRA_COMPONENT_DIRS "gateways/plug_hub")
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_3hub")
	list(APPEND EXTRA_COMPONENT_DIRS "gateways/plug_hub")
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "water_valve")
		list(APPEND EXTRA_COMPONENT_DIRS "gateways/water_valve")
endif()

set(ENV{BUILD_ESP32} 1)

if(NOT BUILD_FOR_LINUX)
	include($ENV{IDF_PATH}/tools/cmake/project.cmake)
endif()

# Add compile options
add_compile_options(
	-DSSL_FREE_CERTIFICATES_MEMORY_AFTER_CONNECTION
	-DLUA_COMPAT_5_2
	-DSNTP_DEBUG=LWIP_DBG_ON
	)
if(NOT BUILD_FOR_LINUX)
	add_compile_options(-DESP_PLATFORM)
	if(BUILD_FOR_FACTORY_TEST)
		add_compile_options(-DFACTORY_TEST)
	endif()
endif()

# Define project
if(BUILD_FOR_FACTORY_TEST)
	project(atom32_factory_test)
	add_compile_definitions(IS_FACTORY_TEST)
else()
	project(atom32)
endif()

if(CONFIG_LOCAL_MODE)
	message(STATUS "Local mode: ON")
else()
	message(STATUS "Local mode: OFF")
endif()

if(CONFIG_CLOUD_DEVICES)
        set(CLOUD_DEVICES "ON")
	message(STATUS "Cloud devices: ON")
else()
	message(STATUS "Cloud devices: OFF")
endif()
if(CONFIG_ADVANCED_SCENES)
	message(STATUS "Advanced scenes: ON")
else()
	message(STATUS "Advanced scenes: OFF")
endif()
if(CONFIG_IS_LOW_MEMORY_DEVICE)
	message(STATUS "Is low-memory device: ON")
else()
	message(STATUS "Is low-memory device: OFF")
endif()

if(NOT BUILD_FOR_LINUX)
	if(PROD_LOGS)
		set(DEFAULT_ESP_LOG_LEVEL ${CONFIG_PROD_ESP_LOG_LEVEL})
	else()
		set(DEFAULT_ESP_LOG_LEVEL ${CONFIG_DEV_ESP_LOG_LEVEL})
	endif()
	# ESP IDF build system adds __idf_ prefix to target names of the components
	target_compile_definitions(__idf_hub_core PRIVATE DEFAULT_ESP_LOG_LEVEL=${DEFAULT_ESP_LOG_LEVEL})
	message(STATUS "ESP log level: ${DEFAULT_ESP_LOG_LEVEL}")
endif()

if(BUILD_FOR_FACTORY_TEST)
#	add_subdirectory(component/factory_test)
else()
#	add_subdirectory(component/main)
endif()

# Function for add built in data
if(BUILD_FOR_LINUX)
	function(target_add_binary_data)	#TODO: implement target_add_binary_data
	endfunction()
endif()

# Target: test, check
enable_testing()
add_subdirectory(tests)

#------------------------------------------------------------------------------
# Target: deploy, deploy_local

if(DEFINED TARGET)
	if(${TARGET} STREQUAL "RC-US")
		set(DEPLOY_SERVER "cloud.ezlo.com")
	elseif(${TARGET} STREQUAL "STAGE")
		set(DEPLOY_SERVER "cloud-stg.ezlo.com")
	elseif(${TARGET} STREQUAL "QA")
		set(DEPLOY_SERVER "cloud-qa.ezlo.com")
	else()
		set(DEPLOY_SERVER "cloud-dev.ezlo.com")
	endif()
else()
	set(DEPLOY_SERVER "cloud-dev.ezlo.com")
endif()

set(VERSION_FILE ${CONFIGURECMAKE_OUT_DIR}/version/firmware_version)
file(READ ${VERSION_FILE} DEPLOY_VERSION)
set(APP_BIN ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_target(deploy
	COMMAND echo "APP_BIN: ${APP_BIN}"
	COMMAND
	${CMAKE_CURRENT_SOURCE_DIR}/deploy.sh
		APP_BIN=${APP_BIN}
		DEPLOY_LOCAL=N
		DEPLOY_DIR=${CMAKE_CURRENT_BINARY_DIR}/deploy
		DEPLOY_VERSION=${DEPLOY_VERSION}
		DEPLOY_BOARD_VERSION=$ENV{DEPLOY_BOARD_VERSION}
		DEPLOY_SERVER=${DEPLOY_SERVER}
	DEPENDS ${APP_BIN}
	)

add_custom_target(deploy_local
	COMMAND echo "APP_BIN: ${APP_BIN}"
	COMMAND
	${CMAKE_CURRENT_SOURCE_DIR}/deploy.sh
		APP_BIN=${APP_BIN}
		DEPLOY_LOCAL=Y
		DEPLOY_DIR=${CMAKE_CURRENT_BINARY_DIR}/deploy_local/updates
		DEPLOY_VERSION=${DEPLOY_VERSION}
		DEPLOY_BOARD_VERSION=$ENV{DEPLOY_BOARD_VERSION}
		DEPLOY_SERVER=${DEPLOY_SERVER}
	DEPENDS ${APP_BIN}
	)

add_custom_target(run_local_updates
	COMMAND 
	${CMAKE_CURRENT_SOURCE_DIR}/local_updater.sh
		LOCAL_WEB_SERVER_DIR=${CMAKE_CURRENT_BINARY_DIR}/deploy_local
	DEPENDS ${APP_BIN}
	)

#------------------------------------------------------------------------------
# Target: check_pretty, pretty

set(FORMAT_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/components
	${CMAKE_CURRENT_SOURCE_DIR}/gateways
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/app_update
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/driver
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/esp32
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/hard
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/heap
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/lwip
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/mdnsserver
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/nvs_flash
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/spiffs
	${CMAKE_CURRENT_SOURCE_DIR}/linuxcomponent/main.c
	${CMAKE_CURRENT_SOURCE_DIR}/tests
	)

set(EXCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/components/common/include/FirmwareVersion.h
	${CMAKE_CURRENT_SOURCE_DIR}/components/lua
	${CMAKE_CURRENT_SOURCE_DIR}/components/miniz
	${CMAKE_CURRENT_SOURCE_DIR}/components/gasonpp
	${CMAKE_CURRENT_SOURCE_DIR}/components/littlefs
	${CMAKE_CURRENT_SOURCE_DIR}/components/znet
	)

add_custom_target(check_pretty
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/pretty.py --action check --paths ${FORMAT_DIRS} --exclude ${EXCLUDE_DIRS}
	)

add_custom_target(pretty
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/pretty.py --action format --paths ${FORMAT_DIRS} --exclude ${EXCLUDE_DIRS}
	)


#------------------------------------------------------------------------------
# Target: archive_lua_script

set(HUBPLUGINS_DIR ${CMAKE_CURRENT_BINARY_DIR}/hubplugins_src)
set(HUBPLUGINS_SUBMODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hubplugins)

set(LIST_LUA_SCRIPTS
	${HUBPLUGINS_DIR}/gateways/zwave
	${HUBPLUGINS_DIR}/extensions/scene_blocks
	)

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_hub")
        list(APPEND LIST_LUA_SCRIPTS
	${HUBPLUGINS_DIR}/gateways/plughub
	)
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_2hub")
        list(APPEND LIST_LUA_SCRIPTS
	${HUBPLUGINS_DIR}/gateways/plughub
	)
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_3hub")
	list(APPEND LIST_LUA_SCRIPTS
			${HUBPLUGINS_DIR}/gateways/plughub
			)
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "water_valve")
        list(APPEND LIST_LUA_SCRIPTS
        ${HUBPLUGINS_DIR}/gateways/water_valve
	)
endif()

if (CLOUD_DEVICES)
        list(APPEND LIST_LUA_SCRIPTS
        ${HUBPLUGINS_DIR}/gateways/cloud_devices
	)
endif()

execute_process(COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE  )

if (NOT ("${GIT_BRANCH}" STREQUAL "qa" OR "${GIT_BRANCH}" STREQUAL "stage"))
   message(STATUS "not QA or STAGE add test_plugin")
   list(APPEND LIST_LUA_SCRIPTS
        ${HUBPLUGINS_DIR}/gateways/test_plugin
	)
endif()

if("${HUBPLUGINS_ARCHIVE_FMT}" STREQUAL "TAR")
	set(ARTYPE tar.zz)
elseif("${HUBPLUGINS_ARCHIVE_FMT}" STREQUAL "TAR_ZZ")
	set(ARTYPE tar)
elseif("${HUBPLUGINS_ARCHIVE_FMT}" STREQUAL "MOLFAR_ZZ")
	set(ARTYPE molfar.zz)
else()
	message(FATAL_ERROR "HUBPLUGINS_ARCHIVE_FMT option has invalid value: \"${HUBPLUGINS_ARCHIVE_FMT}\", possible values are \"TAR\", \"TAR_ZZ\", \"MOLFAR_ZZ\"")
endif()

execute_process(
		COMMAND git clean -dfX
		WORKING_DIRECTORY ${HUBPLUGINS_SUBMODULE_DIR}
		RESULT_VARIABLE HUBPLUGINS_CLEAN_STATUS
		ERROR_VARIABLE HUBPLUGINS_CLEAN_ERROR
		OUTPUT_QUIET)
if(NOT HUBPLUGINS_CLEAN_STATUS EQUAL 0)
	message(FATAL_ERROR ${HUBPLUGINS_CLEAN_ERROR})
endif()

file(GLOB HUBPLUGINS_SRC ${HUBPLUGINS_SUBMODULE_DIR}/*)
file(REMOVE_RECURSE ${HUBPLUGINS_DIR})
file(COPY ${HUBPLUGINS_SRC} DESTINATION ${HUBPLUGINS_DIR})
file(REMOVE_RECURSE ${HUBPLUGINS_DIR}/gateways/zwave/scripts/tests)

foreach(f ${LIST_LUA_SCRIPTS})
	get_filename_component(basename ${f} NAME_WE)
	set(LUA_SCRIPTS_AR ${CMAKE_CURRENT_BINARY_DIR}/${basename}.${ARTYPE})
	list(APPEND LIST_LUA_SCRIPTS_AR ${LUA_SCRIPTS_AR})
	target_add_binary_data(${PROJECT_NAME}.elf ${LUA_SCRIPTS_AR} BINARY)
	
	file(GLOB_RECURSE LIST_LUA_SCRIPTS_RESOURCES
		${f}/*
		)
	list(APPEND LIST_LUA_SCRIPTS_ALL_RESOURCES ${LIST_LUA_SCRIPTS_RESOURCES})
endforeach()

include(cmake/luac.cmake)

string(RANDOM LENGTH 32 LUAC_TMP_DIR)
string(PREPEND LUAC_TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/)
message(STATUS "Tmp dir for LuaC: ${LUAC_TMP_DIR}")
message(STATUS "LuaC: ${LUAC_BIN}")

add_custom_command(OUTPUT ${LIST_LUA_SCRIPTS_AR} ${CMAKE_CURRENT_SOURCE_DIR}/components/scriptholder/scriptslist.h
	COMMAND cd ${HUBPLUGINS_DIR}/gateways && python3 -B dev_utils/device_generation.py
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/components/scriptholder &&
			${CMAKE_CURRENT_SOURCE_DIR}/components/scriptholder/compress.py
				--pluginlist ${LIST_LUA_SCRIPTS}
				--artype=${ARTYPE}
				--arpath=${CMAKE_CURRENT_BINARY_DIR}
				--tmppath ${LUAC_TMP_DIR}
				--luac ${LUAC_BIN}
				--plugdir ${CMAKE_CURRENT_SOURCE_DIR}/hubplugins
	DEPENDS LuaC ${LIST_LUA_SCRIPTS_ALL_RESOURCES}
	)

add_custom_target(archive_lua_script
	DEPENDS ${LIST_LUA_SCRIPTS_AR}
	)

if(NOT BUILD_FOR_LINUX)
	add_dependencies(${PROJECT_NAME}.elf archive_lua_script)
endif()
