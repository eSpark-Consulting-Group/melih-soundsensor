#pragma once

#include "Shutdownable.h"

#include <lua.hpp>
#include "LuaModule.h"
#include "LuaModuleCloud.h"
#include "LuaModulePlugHub.h"
#include "LuaModuleWaterValve.h"
#include "LuaModuleScriptDebug.h"
#include "LuaModuleStorage.h"
#include "LuaModuleCore.h"
#include "LuaModuleTimers.h"
#include "LuaModuleLogging.h"
#include "LuaModuleZWave.h"
#include "LuaModuleScenes.h"
#include "plugin_info.h"
#include <plugin_manager.h>

#include <StorageArFile.h>
#include <StorageZipFile.h>

#include <list>
#include <string>

namespace gateways {
class IGatewayUserDataStorage;
}

namespace lua {

class Executor final : public core::SingletonBaseImpl<Executor> {
    friend class IModule;
    friend class module::ScriptDebug;
    friend class core::SingletonBaseImpl<Executor>;

public:
    using ParentType = core::SingletonBaseImpl<Executor>;
    using ClbkRet = ParentType::ClbkRet;
    using UpdContext = core::upd_srv::UpdContext;

    static constexpr auto MODULE_CACHE_TABLE = "_LOADED";  // Use the same name as native Lua "require"

    static void threadTask( std::shared_ptr<Executor> executor );

    ~Executor();

    inline const plugin::PluginID& getCurrentPluginID() const {
        return m_currentPluginID;
    }

    /// INFO: used in IModule->IBusCommandVisitor->visit: load lua object to lua stack and return count of elements
    /// pushed to lua stack
    inline lua_State* lua() {
        return m_lua;
    }

    inline void luaRet( int luaRet ) {
        m_luaRet = luaRet;
    }

    // for Modules
    bool busJoin( IModule& mod,
                  const char* addrName,
                  const char* file = nullptr,
                  const plugin::PluginInfo& plugin = plugin::PluginInfo() );
    void busLeave( IModule& mod,
                   const char* addrName,
                   const char* file = nullptr,
                   const plugin::PluginInfo& plugin = plugin::PluginInfo() );

    // LUA API
    static int luaRequire( lua_State* lua );

    static int luaLoadfile( lua_State* lua );

    static const char* codeReader( lua_State* lua, void* ud, size_t* osize );

    std::tuple<bool, common::IValue::UPtr> execScript( const std::string& script,
                                                       std::function<int( lua_State* )> push_params_clbk );

    bool initializeMods();

    ::gateways::IGatewayUserDataStorage& getGatewayUserDataStorage() const {
        return m_gatewayUserDataStorage;
    }

private:
    // bus::IBusClient
    void receive( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommand& msg ) override;

    // IBusCommandVisitor
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::HandleEventUpdateProductName& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::HubSettingsApply& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueUpdated& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueSet& command );
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetConsumedEnergyMeter& command );
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::ElectricMeterValuesUpdated& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ResetEnergyMeter& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::WaterLeakValueUpdate& command ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::cloud::responses::HandleEventCloudDeviceAdded& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::cloud::responses::HandleEventCloudDeviceRemoved& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::cloud::responses::HandleEventCloudDeviceUpdated& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::cloud::responses::HandleEventCloudResponseCommand& ) override;
    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::command::SendGenericEventToLuaHandler& ) override;

    // for Modules
    bool reg( IModule& mod, const char* name );

    void regApi( lua_State* lua );

    int regModule( const char* name, lua_State* lua );

    bool callReceiver( IModule* mod, const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommand& msg );

private:
    Executor(::gateways::IGatewayUserDataStorage& stor,
             const std::chrono::milliseconds& shutdownDelay,
             const unsigned int msgLen );

    ClbkRet updateComponent( UpdContext& ctx ) override;

    static bool loadModuleFromCache( lua_State* lua, const char* name );
    static void storeModuleToCache( lua_State* lua, const char* name );

    struct LuaCleaner {
        Executor& m_owner;
        void operator()( lua_State* lua );
    };

    typedef std::unique_ptr<lua_State, LuaCleaner> LuaInstGuard;

    lua_State* createLuaInstance();
    LuaInstGuard getOrCreateLuaInstance();
    void closeLuaInstance();
    bool loadScript( const std::string& script_name, bool root );

    storage::ArchiveFile* getOrCreateArchiveFile( const std::string& archive_uri, const std::string& file_name );

    void handle(::bus::BusAddr, ::bus::BusAddr, ::bus::command::HandleEventBase& command );

private:
    lua_State* m_lua = nullptr;
    int m_luaRet = 0;

    struct Receiver {
        const bus::BusAddr to;
        // mod
        IModule* mod;
        // script
        const plugin::PluginInfo plugin;
        const std::string file;
        // ref counter
        size_t joinCount;

        Receiver( const bus::BusAddr _to, IModule& _mod, const plugin::PluginInfo _plugin, const char* _file )
        : to( _to )
        , mod( &_mod )
        , plugin( _plugin )
        , file( _file )
        , joinCount( 0 ) {}
    };
    std::list<Receiver> m_receivers;

    struct Module {
        const char* name;
        IModule* mod;

        Module( const char* _name, IModule& _mod )
        : name( _name )
        , mod( &_mod ) {}
    };
    std::list<Module> m_mods;

    // modules
    module::ScriptDebug m_scriptDebugMod;
    module::Storage m_storageMod;
    module::ZWave m_zwaveMod;
#ifdef CONFIG_CLOUD_DEVICES
    cloud::LuaModule m_cloudMod;
#endif
    module::Core m_coreMod;
    module::Timers m_timerMod;
#if CONFIG_REMOTE_LOGGING
    module::Logging m_loggingMod;
#endif  // CONFIG_REMOTE_LOGGING
#if CONFIG_BOARD_TYPE_PLUG_HUB
    module::luaPlugHub m_PlugHubMod;
#endif
#if CONFIG_BOARD_TYPE_WATER_VALVE
    module::luaWaterValve m_WaterValveMod;
#endif
    module::luaScenes m_scenesMod;

    static constexpr int BUFFER_SIZE = 256;
    std::vector<char> m_buffer;

    std::map<std::string, std::unique_ptr<storage::ArchiveFile> > m_files;

    storage::ArchiveFile* m_currentFile = nullptr;

    plugin::PluginID m_currentPluginID;
    plugin::PluginManager::RevisionChecker m_luaCacheRevisionChecker;
    plugin::PluginManager::RevisionChecker m_filesRevisionchecker;

    ::gateways::IGatewayUserDataStorage& m_gatewayUserDataStorage;
};  // class Executor

}  // namespace lua
