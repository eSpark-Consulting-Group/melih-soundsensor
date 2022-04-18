#pragma once

#include <LuaModule.h>

namespace lua {

namespace module {

class luaScenes final : public IModule {
    static constexpr auto SCENES_NAME = "scenes";

public:
    virtual ~luaScenes() = default;
    luaScenes( Executor& exec, const char* name );

    bool initialize() override;

    int regModule( lua_State* lua ) override;
};

}  // namespace module

}  // namespace lua
