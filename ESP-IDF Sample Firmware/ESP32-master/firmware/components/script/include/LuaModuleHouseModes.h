#pragma once

#include <LuaModule.h>

namespace lua {

namespace module {

class HouseModes final : public IModule {
public:
    virtual ~HouseModes() = default;
    HouseModes( Executor& exec, const char* name );

    int regModule( lua_State* lua ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ModesSwitched& command ) override;
};

}  // namespace module

}  // namespace lua
