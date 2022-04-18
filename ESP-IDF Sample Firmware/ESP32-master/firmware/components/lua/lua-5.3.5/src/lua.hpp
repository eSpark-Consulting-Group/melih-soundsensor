// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

// May be required in case if GNU's limit.h is missing
#include <climits>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
