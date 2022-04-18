set(SOURCES
    src/addon_manager.cpp
	src/plugin_downloader.cpp
	src/plugin_config.cpp
	src/plugin_manager.cpp
	src/plugin_repository.cpp
	)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES common storage scriptholder miniz)

include(../config_flags_throw.cmake)