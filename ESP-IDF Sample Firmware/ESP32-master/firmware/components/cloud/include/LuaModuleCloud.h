#pragma once

#ifdef CONFIG_CLOUD_DEVICES

#include <LuaModule.h>

namespace cloud {

namespace responces {
class HandleEventCloudResponseCommand;
class HandleEventCloudDeviceAdded;
class HandleEventCloudDeviceRemoved;
class HandleEventCloudDeviceUpdated;
}  // namespace responces

class LuaModule final : public lua::IModule {
    static constexpr auto CLOUD_NAME = "cloud_devices";

public:
    using ParentType = lua::IModule;

    LuaModule( lua::Executor& exec, const char* name );

    bool initialize() override;

    bool onLoop();
    int regModule( lua_State* lua ) override;

    static int luaJoin( lua_State* lua );
    static int luaSubscribe( lua_State* lua );
    static int luaUnsubscribe( lua_State* lua );
    static int luaRequestAllValues( lua_State* lua );

    static int getAllDevicesIds( lua_State* lua );
    static int sendCommand( lua_State* luaState );
    static int getDevice( lua_State* luaState );

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                responses::HandleEventCloudResponseCommand& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                responses::HandleEventCloudDeviceAdded& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                responses::HandleEventCloudDeviceRemoved& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                responses::HandleEventCloudDeviceUpdated& command ) override;
};

}  // namespace cloud

#endif  // CONFIG_CLOUD_DEVICES
