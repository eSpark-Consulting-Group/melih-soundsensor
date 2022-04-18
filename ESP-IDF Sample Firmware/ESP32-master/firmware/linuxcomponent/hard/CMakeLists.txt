add_library(hard STATIC
	src/LedDevice.cpp
	src/Reset.cpp
	src/Wifi.cpp
        src/HardUtils.cpp
	)

target_include_directories(hard
	PUBLIC include
	)

target_link_libraries(hard common esp32 soc heap freertos newlib )
