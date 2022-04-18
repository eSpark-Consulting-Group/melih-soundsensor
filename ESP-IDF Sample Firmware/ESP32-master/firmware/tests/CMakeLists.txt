cmake_minimum_required(VERSION 3.10)

if(NOT BUILD_FOR_LINUX)
    message(STATUS "Skipping unit tests target: build for linux is disabled")
    return()
endif()

include(External_GTest.cmake)
include(GoogleTest)

file(GLOB TEST_SOURCES *.cpp)
add_executable(unittests EXCLUDE_FROM_ALL ${TEST_SOURCES})
target_link_libraries(unittests gtest_main gtest gmock pthread)
gtest_discover_tests(unittests)

# Enable 'make check' instead of 'make test'.
# https://gitlab.kitware.com/cmake/cmake/-/issues/8774
add_custom_target(check
    COMMAND env CTEST_OUTPUT_ON_FAILURE=1 GTEST_COLOR=1 ${CMAKE_CTEST_COMMAND} -T test --no-compress-output -V
    DEPENDS unittests)
