# This file defines LuaC target that builds Lua compiler which is required by 'archive_lua_script' target.
# The resulting binary file is located at ${LUAC_BIN}.
#
# We have to copy sources and perform in-source build to keep the original dir structure, which is
# required by the top-level LuaC Makefile. There's no another way to workaround this.

cmake_minimum_required(VERSION 3.0.2)

if(${CMAKE_HOST_UNIX})
    set(LUAC_BUILD_TARGET linux)
elseif(${CMAKE_HOST_WIN32})
    set(LUAC_BUILD_TARGET mingw)
else()
    set(LUAC_BUILD_TARGET generic)
endif()

find_file(MAKE_EXE make)
if(${MAKE_EXE} MATCHES ".+-NOTFOUND")
    message(FATAL_ERROR "Can't find make program")
endif()

message(STATUS "Found make: ${MAKE_EXE}")
message(STATUS "LuaC build target: ${LUAC_BUILD_TARGET}")

if(CONFIG_IS_LOW_MEMORY_DEVICE)
    set(LUAC_INT_MODE -DLUA_32BITS)
    message(STATUS "LuaC is forced to 32-bit mode")
else()
    set(LUAC_INT_MODE)
endif()

set(LUAC_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/components/lua/lua-5.3.5)
set(LUAC_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/LuaC)
set(LUAC_MAKEFILE ${LUAC_BUILD_DIR}/Makefile)
set(LUAC_BIN ${LUAC_BUILD_DIR}/src/luac)

# We have to copy sources and perform in-source build to keep the original dir structure, which is
# required by the top-level LuaC Makefile. There's no another way to workaround this.
file(GLOB LUAC_SOURCES ${LUAC_SRC_DIR}/*)
file(COPY ${LUAC_SOURCES} DESTINATION ${LUAC_BUILD_DIR})

include(ExternalProject)

ExternalProject_Add(
        LuaC
        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND ""
        BINARY_DIR ${LUAC_BUILD_DIR}
        SOURCE_DIR ${LUAC_SRC_DIR}
        BUILD_COMMAND ${MAKE_EXE} ${LUAC_BUILD_TARGET} LUAC_INT_MODE=${LUAC_INT_MODE} -f ${LUAC_MAKEFILE})
