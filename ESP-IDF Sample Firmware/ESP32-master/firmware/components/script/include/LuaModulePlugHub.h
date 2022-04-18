#pragma once

#include <LuaModule.h>

namespace lua {

namespace module {

class luaPlugHub final : public IModule {
    static constexpr auto PLUG_HUB_NAME = "plughub";

public:
    virtual ~luaPlugHub() = default;
    luaPlugHub( Executor& exec, const char* name );

    bool initialize() override;

    int regModule( lua_State* lua ) override;

    // LUA API
    static int luaJoin( lua_State* lua );
    static int luaSubscribe( lua_State* lua );
    static int luaUnsubscribe( lua_State* lua );

    static int luaRequestAllValues( lua_State* lua );
    static int luaSetSwitchValue( lua_State* lua );
    static int luaResetConsumedEnergyMeter( lua_State* lua );
    static int luaSetConsumedEnergyMeter( lua_State* lua );

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueUpdated& command );
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ElectricMeterValuesUpdated& command );
};

}  // namespace module

}  // namespace lua
