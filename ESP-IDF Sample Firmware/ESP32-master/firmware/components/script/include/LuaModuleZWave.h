#pragma once

#include <LuaModule.h>

namespace lua {

namespace module {

class ZWave final : public IModule {
    static constexpr auto ZNET_NAME = "znet";

public:
    virtual ~ZWave() = default;
    ZWave( Executor& exec, const char* name );

    bool initialize() override;

    int regModule( lua_State* lua ) override;

    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::SendGenericEventToLuaHandler& );
};

}  // namespace module

}  // namespace lua
