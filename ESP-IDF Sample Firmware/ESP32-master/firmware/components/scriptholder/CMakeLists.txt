idf_component_register(SRCS "src/scriptholder.c"
                       INCLUDE_DIRS "include" "."
                       REQUIRES lua
                       )
add_dependencies(${COMPONENT_LIB} LuaC archive_lua_script)
include(../config_flags_throw.cmake)