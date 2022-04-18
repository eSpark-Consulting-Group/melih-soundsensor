idf_component_register(SRCS "src/JsonListener.cpp" "src/JsonStreamingParser.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES common)
include(../config_flags_throw.cmake)