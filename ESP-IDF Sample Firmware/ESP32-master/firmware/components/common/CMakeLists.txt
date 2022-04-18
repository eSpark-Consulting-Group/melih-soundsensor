file(GLOB_RECURSE SOURCES "src/*.c*")
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES hub_core network freertos esp32 soc heap newlib log nvs_flash spi_flash)
include(../config_flags_throw.cmake)