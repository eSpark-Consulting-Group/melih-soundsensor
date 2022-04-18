#include "LuaModuleLogging.h"

#if CONFIG_REMOTE_LOGGING

#include "Lua.h"
#include "bindings/LuaBindingHelper.h"

#include "bindings/LuaLoggingBindings.hpp"

namespace lua {

namespace module {
Logging::Logging( Executor& exec, const char* name )
: IModule( exec, name ) {}

bool Logging::initialize() {
    return true;
}

int Logging::regModule( lua_State* lua ) {
    static const auto bindArray = lua::createBindingsMap(

        luaL_Reg{ "get_level", bindings::luaBinding<bindings::logging::GetLevel> },
        luaL_Reg{ "log", bindings::luaBinding<bindings::logging::Log> },
        luaL_Reg{ "trace", bindings::luaBinding<bindings::logging::Trace> },
        luaL_Reg{ "debug", bindings::luaBinding<bindings::logging::Debug> },
        luaL_Reg{ "info", bindings::luaBinding<bindings::logging::Info> },
        luaL_Reg{ "warning", bindings::luaBinding<bindings::logging::Warning> },
        luaL_Reg{ "error", bindings::luaBinding<bindings::logging::Error> } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

}  // end of namespace module

}  // end of namespace lua

#endif  // CONFIG_REMOTE_LOGGING
