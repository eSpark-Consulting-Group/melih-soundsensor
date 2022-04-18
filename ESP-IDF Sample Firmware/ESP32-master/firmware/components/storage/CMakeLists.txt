set(STORAGE_SRC
	src/SettingStorage.cpp
	src/StorageArFile.cpp
	src/StorageArchiveFileFactory.cpp
	src/Storage.cpp
	src/StorageFile.cpp
	src/BinaryFileStorageFactory.cpp
	src/BinaryFileStorage.cpp
	src/StorageFileFormat.cpp
	src/StorageSection.cpp
	src/StorageCommonSection.cpp
	src/SystemStorage.cpp
	src/FileStream.cpp
	src/Volume.cpp
	src/crc16.c
	src/KeyValueStorage.cpp)

set(STORAGE_LINK_LIBRARIES nvs_flash common esp_littlefs)

if("$ENV{HUBPLUGINS_ARCHIVE_FMT}" STREQUAL "TAR_ZZ")
	message(STATUS "Add support for ZIP archives (tar.zz)")
	list(APPEND STORAGE_SRC src/StorageZipFile.cpp)
	list(APPEND STORAGE_LINK_LIBRARIES miniz)
elseif("$ENV{HUBPLUGINS_ARCHIVE_FMT}" STREQUAL "MOLFAR_ZZ")
	message(STATUS "Add support for MOLFAR archives (molfar.zz)")
	list(APPEND STORAGE_SRC src/StorageMolfarFile.cpp)
else()
	message(STATUS "Support of storage archives is disabled")
endif()

idf_component_register(SRCS ${STORAGE_SRC}
                       INCLUDE_DIRS "include" "."
                       REQUIRES ${STORAGE_LINK_LIBRARIES}
)

if("$ENV{HUBPLUGINS_ARCHIVE_FMT}" STREQUAL "TAR_ZZ" )
	target_compile_definitions(${COMPONENT_TARGET} PUBLIC "-DZIP_ARCHIVE=1")
elseif("$ENV{HUBPLUGINS_ARCHIVE_FMT}" STREQUAL "MOLFAR_ZZ")
	target_compile_definitions(${COMPONENT_TARGET} PUBLIC "-DMOLFAR_FORMAT")
endif()

include(../config_flags_throw.cmake)
