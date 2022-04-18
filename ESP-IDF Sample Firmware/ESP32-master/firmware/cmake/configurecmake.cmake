cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED CONFIGURECMAKE_OUT_DIR)
    set(CONFIGURECMAKE_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmakeconfig")
endif()

if(DEFINED CMAKE_CONFIG_BOARD_TYPE)
    if(DEFINED CMAKE_CONFIG_INITIALBRANCH)
        set(CMAKE_CONFIG_INITIALBRANCH_ARG_STR "--initialbranch=${CMAKE_CONFIG_INITIALBRANCH}")
    else()
        set(CMAKE_CONFIG_INITIALBRANCH_ARG_STR "")
    endif()
    set(CONFIGURECMAKE_CMD configurecmake.py ${CMAKE_CONFIG_INITIALBRANCH_ARG_STR} --board=${CMAKE_CONFIG_BOARD_TYPE} --config-dir=${CONFIGURECMAKE_OUT_DIR})
    string(REPLACE ";" " " CONFIGURECMAKE_CMD_STR "${CONFIGURECMAKE_CMD}")
    message(STATUS ${CONFIGURECMAKE_CMD_STR})
    execute_process(
            COMMAND ${CMAKE_COMMAND} -E env PYTHONDONTWRITEBYTECODE=1 python3 ${CONFIGURECMAKE_CMD}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE CONFIGURECMAKE_STATUS
            ERROR_VARIABLE CONFIGURECMAKE_ERROR
            OUTPUT_QUIET)
    if(NOT CONFIGURECMAKE_STATUS EQUAL 0)
        message(FATAL_ERROR ${CONFIGURECMAKE_ERROR})
    endif()
else()
    message(STATUS "CMAKE_CONFIG_BOARD_TYPE is not defined")
endif()

# Adds cmake definitions of board type name & version numbers
if(EXISTS ${CONFIGURECMAKE_OUT_DIR}/config.cmake)
    include(${CONFIGURECMAKE_OUT_DIR}/config.cmake)
else()
    message(FATAL_ERROR
            " config.cmake is not found at the following location: ${CONFIGURECMAKE_OUT_DIR}\n"
            " Either set CMAKE_CONFIG_BOARD_TYPE or execute configurecmake.py manually.")
endif()

# Required to properly initialize esp-idf/tools/cmake/project.cmake
set(SDKCONFIG ${CONFIGURECMAKE_OUT_DIR}/sdkconfig)
set(ENV{SDKCONFIG_DEFAULTS} "${CONFIGURECMAKE_OUT_DIR}/sdkconfig.defaults")

# Adds location of FirmwareVersion.h
include_directories(${CONFIGURECMAKE_OUT_DIR}/version/include)
