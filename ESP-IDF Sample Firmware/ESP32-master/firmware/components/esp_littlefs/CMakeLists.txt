idf_component_register(SRCS "src/esp_littlefs.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES littlefs nvs_flash bootloader_support vfs freertos heap newlib esp32)
include(../config_flags_throw.cmake)