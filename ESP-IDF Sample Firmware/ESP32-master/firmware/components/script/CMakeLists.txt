file(GLOB lua_converters_sources src/converters/*.cpp)

set(files
		src/Lua.cpp
		src/LuaBindingHelper.cpp
		src/LuaEventManager.cpp
		src/LuaModule.cpp
		src/LuaModuleCore.cpp
		src/LuaModuleHouseModes.cpp
		src/LuaModuleLogging.cpp
		src/LuaModuleScenes.cpp
		src/LuaModuleScriptDebug.cpp
		src/LuaModuleStorage.cpp
		src/LuaModuleTimers.cpp
		src/LuaModuleZWave.cpp
		src/LuaTableSerializer.cpp
		${lua_converters_sources})

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_hub")
	list(APPEND files src/LuaModulePlugHub.cpp)
endif()	

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_2hub")
	list(APPEND files src/LuaModulePlugHub.cpp)
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "plug_3hub")
	list(APPEND files src/LuaModulePlugHub.cpp)
endif()

if($ENV{DEPLOY_BOARD_VERSION} STREQUAL "water_valve")
	list(APPEND files src/LuaModuleWaterValve.cpp)
endif()	

idf_component_register(SRCS ${files}
                       INCLUDE_DIRS "include"
                       REQUIRES common lua cloud scriptholder)
include(../config_flags_throw.cmake)