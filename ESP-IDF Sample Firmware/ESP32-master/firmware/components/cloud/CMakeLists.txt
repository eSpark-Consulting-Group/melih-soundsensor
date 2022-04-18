set(SOURCES
	src/CloudDevicesManager.cpp
	src/devices/CloudDevice.cpp
	src/CloudGeteway.cpp
	src/CloudDevicesStorage.cpp
	src/LuaModuleCloud.cpp
)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include" "include/devices"
                       REQUIRES common model)
include(../config_flags_throw.cmake)
