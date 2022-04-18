#pragma once

#include <lua.hpp>

#include <LuaModule.h>
#include <KeyValueStorage.h>
#include <DeviceInfo.h>

#include <vector>
#include <string>

namespace lua {

namespace module {

class Core final : public IModule {
public:
    virtual ~Core() = default;
    Core( Executor& exec, const char* name );

    bool initialize() override;

    int regModule( lua_State* lua ) override;

    static int luaAddDevice( lua_State* lua );
    static int luaRemoveDevice( lua_State* lua );
    static int luaRemoveItem( lua_State* lua );
    static int luaRemoveItemDictionaryValue( lua_State* lua );
    static int luaModifyItem( lua_State* lua );
    static int luaRemoveGatewayDevices( lua_State* lua );
    static int luaGetDevice( lua_State* lua );
    static int luaGetDevices( lua_State* lua );
    static int luaModifyDevice( lua_State* lua );
    static int luaAddItem( lua_State* lua );
    static int luaGetItem( lua_State* lua );
    static int luaGetItems( lua_State* lua );
    static int luaGetItemsByDeviceId( lua_State* lua );
    static int luaUpdateItemDictionaryValue( lua_State* lua );
    static int luaUpdateReachableState( lua_State* lua );
    static int luaUpdateDeviceStatus( lua_State* lua );
    static int luaUpdateReadyState( lua_State* lua );
    static int luaSendUiBroadcast( lua_State* lua );
    static int luaGetProductName( lua_State* lua );
    static int luaGenerateItemDictionaryNumberId( lua_State* lua );
    static int sendDeviceFirmwareUpdateBroadcast( lua_State* lua );
    static int sendDeviceFirmwareDownloadBroadcast( lua_State* lua );

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::HandleEventUpdateProductName& command );
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::HubSettingsApply& command );

    static void processItemsByDeviceId( const char* deviceId, const std::function<void( const char* itemId )>& func );
    static void sendReachableState( Executor& executor, const char* device_id, const bool reachable );

private:
    static void fillItemsListByDeviceId( lua_State* lua,
                                         std::vector<std::string>& list,
                                         const char* deviceId = nullptr );
    static std::vector<std::string> getChildDevicesByParentDeviceId( const char* deviceId = nullptr );
    static void processDevicesByGatewayId( const char* gatewayId,
                                           const std::function<void( const char* deviceId )>& func );

    static void processSettingsByDeviceId( const char* deviceId,
                                           const std::function<void( const char* settingId )>& func );
    static void pushDeviceToLua( lua_State* lua, const gateways::BaseDeviceInfo& devicInfo );
    static void pushItemToLua( lua_State* lua, const gateways::ItemInfo& itemInfo );
    static Executor& executorFromLua(::lua_State* );

    static void readTypeSpecificFieldsFromLua( lua_State* lua, ::gateways::ItemInfo& itemInfo );
    static bool canSendItemUpdatedRequestForItem( const ::gateways::ItemInfo& itemInfo );

    static bool hubSettingsApplyTemperatureScale( std::string dstScale );
    static bool hubSettingsApplyTemperatureScaleSettings( std::string dstScale );
    static void setInfo( lua_State* lua, std::unique_ptr<gateways::BaseDeviceInfo>& devicesInfo );
};

}  // namespace module

}  // namespace lua
