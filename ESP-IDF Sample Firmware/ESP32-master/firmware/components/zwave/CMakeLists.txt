set(SOURCES
	src/ProvisioningListEntrySerializer.cpp
	src/ProvisioningListStorage.cpp
	src/ZWaveDevicesStorage.cpp
	src/ZWaveDeviceSetting.cpp
	src/ZWaveGateway.cpp
	src/ZWaveResources.cpp
	src/ZWaveResults.cpp
	src/ZWaveZNet.cpp
	src/anti_theft_helper.cpp
	src/devices/ZWaveDevice.cpp
	../storage/src/BinaryFile.cpp
	src/zwave_device_info.cpp
	)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES common model extension driver)
include(../config_flags_throw.cmake)