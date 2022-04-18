cmake_minimum_required(VERSION 3.10)

include(ExternalProject)

set(gtest_BUILD_PREFIX "${CMAKE_BINARY_DIR}/googletest")
set(gtest_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/googletest/googletest")
set(gmock_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/googletest/googlemock")

if(NOT EXISTS "${gtest_SOURCE_DIR}" OR NOT EXISTS "${gmock_SOURCE_DIR}")
    message(FATAL_ERROR "Google Test sources not found. Probably the submodule has not been initialized yet.")
endif()

ExternalProject_Add(gtest_from_src
    PREFIX "${gtest_BUILD_PREFIX}"
    SOURCE_DIR "${gtest_SOURCE_DIR}"
    BINARY_DIR "${gtest_BUILD_PREFIX}/gtest"
    INSTALL_COMMAND ""
    DOWNLOAD_COMMAND ""
    UPDATE_COMMAND ""
)

ExternalProject_Add(gmock_from_src
    PREFIX "${gtest_BUILD_PREFIX}"
    SOURCE_DIR "${gmock_SOURCE_DIR}"
    BINARY_DIR "${gtest_BUILD_PREFIX}/gmock"
    INSTALL_COMMAND ""
    DOWNLOAD_COMMAND ""
    UPDATE_COMMAND ""
)

add_library(gtest STATIC IMPORTED)
set_property(TARGET gtest PROPERTY IMPORTED_LOCATION  ${gtest_BUILD_PREFIX}/gtest/libgtest.a)

add_library(gtest_main STATIC IMPORTED)
set_property(TARGET gtest_main PROPERTY IMPORTED_LOCATION  ${gtest_BUILD_PREFIX}/gtest/libgtest_main.a)

add_library(gmock STATIC IMPORTED)
set_property(TARGET gmock PROPERTY IMPORTED_LOCATION  ${gtest_BUILD_PREFIX}/gmock/libgmock.a)

add_library(gmock_main STATIC IMPORTED)
set_property(TARGET gmock_main PROPERTY IMPORTED_LOCATION  ${gtest_BUILD_PREFIX}/gmock/libgmock_main.a)

add_dependencies(gtest gtest_from_src)
add_dependencies(gtest_main gtest_from_src)
add_dependencies(gmock gmock_from_src)
add_dependencies(gmock_main gmock_from_src)

include_directories("${gtest_SOURCE_DIR}/include")
include_directories("${gmock_SOURCE_DIR}/include")
