#pragma once

#include <LuaModule.h>

namespace lua {

namespace module {

class luaWaterValve final : public IModule {
    static constexpr auto WATER_VALVE_NAME = "watervalve";

public:
    virtual ~luaWaterValve() = default;
    luaWaterValve( Executor& exec, const char* name );

    bool initialize() override;

    int regModule( lua_State* lua ) override;

    // LUA API
    static int luaJoin( lua_State* lua );
    static int luaSubscribe( lua_State* lua );
    static int luaUnsubscribe( lua_State* lua );

    static int luaRequestAllValues( lua_State* lua );
    static int luaSetSwitchValue( lua_State* lua );
    static int luaGetWaterLeakSensorCount( lua_State* lua );
    static int luaShould_invert_control( lua_State* lua );

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueUpdated& command );
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::WaterLeakValueUpdate& command );
};

}  // namespace module

}  // namespace lua
