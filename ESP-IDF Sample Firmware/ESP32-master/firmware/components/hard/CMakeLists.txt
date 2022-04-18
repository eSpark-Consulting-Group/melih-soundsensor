idf_component_register(SRCS "src/FlashUtils.cpp" "src/LedDevice.cpp" 
                            "src/MemoryMappedFlash.cpp" "src/ResetZW.cpp" 
                            "src/HardUtils.cpp" "src/GpioButton.cpp" "src/Meter_cc.cpp" "src/LedControl.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES common esp32 soc heap freertos newlib)
include(../config_flags_throw.cmake)