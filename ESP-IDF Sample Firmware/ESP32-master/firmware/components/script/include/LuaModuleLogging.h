#pragma once

#if CONFIG_REMOTE_LOGGING

#include <lua.hpp>

#include <LuaModule.h>

#include <vector>
#include <string>

namespace lua {

namespace module {
class Logging final : public IModule {
public:
    Logging( Executor& exec, const char* name );

    bool initialize() override;

    int regModule( lua_State* lua ) override;
};
}  // namespace module

}  // namespace lua

#endif  // CONFIG_REMOTE_LOGGING
