#include <Lua.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <list>

#include <freertos/task.h>
#include <esp_heap_caps.h>

#include <Ezlo.h>
#include <StorageArFile.h>
#include <StorageZipFile.h>
#include <BinaryFileStorageFactory.h>
#include <StoragePartitionLabel.h>
#include <DebugUtils.h>
#include <plugin_manager.h>
#include <StorageArchiveFileFactory.h>
#include <BusCommandHandleEvent.h>
#include <BusCommandHandleEventUpdateProductName.hpp>
#include <BusCommandHubSettingsApply.hpp>
#include <BusCommandSwitchValueUpdated.hpp>
#include <BusCommandSwitchValueSet.hpp>
#include <BusCommandSetConsumedEnergyMeter.hpp>
#include <BusCommandElectricMeterValuesUpdated.hpp>
#include <BusCommandResetEnergyMeter.hpp>
#include <BusCommandWaterLeakValueUpdate.hpp>
#include <BusCommandScheduleEntryLock.hpp>
#include <BusCommandSoundSwitch.hpp>
#include <lua/BusCommandSendEventToLuaHandler.hpp>

#include <cloud/responses/BusCommandHandleEventCloudResponseCommand.h>
#include <cloud/responses/BusCommandHandleEventCloudDeviceAdded.h>
#include <cloud/responses/BusCommandHandleEventCloudDeviceRemoved.h>
#include <cloud/responses/BusCommandHandleEventCloudDeviceUpdated.h>

#include <CommonResources.h>
#include "EspLogTags.h"
#include "esp_heap_caps.h"
#include "Thread.h"

//#define SIMPLE_LUA_DEBUG

#define LUA_LOGE( ... ) ESP_LOGE(::esp_log_tags::LUA, __VA_ARGS__ )

#ifdef SIMPLE_LUA_DEBUG

#include <esp_log.h>
#include "EspLogTags.h"

#define LUA_LOGD( ... ) ESP_LOGD(::esp_log_tags::LUA, __VA_ARGS__ )
#define LUA_LOGI( ... ) ESP_LOGI(::esp_log_tags::LUA, __VA_ARGS__ )
//#define LUA_LOGE( ... ) ESP_LOGE(::esp_log_tags::LUA, __VA_ARGS__ )
#define LUA_LOGW( ... ) ESP_LOGW(::esp_log_tags::LUA, __VA_ARGS__ )

#else

#define LUA_LOGD( ... ) \
    do {                \
    } while( false )
#define LUA_LOGI( ... ) \
    do {                \
    } while( false )
#define LUA_LOGW( ... ) \
    do {                \
    } while( false )

#endif

namespace lua {

namespace {

int luaPanic( lua_State* lua ) {
    REMOTE_LOGE( "PANIC: unprotected error in call to Lua API (%s)", lua_tostring( lua, -1 ) );
    return 0;
}

static const luaL_Reg defaultLibs[] = { { "_G", luaopen_base },
                                        // { LUA_LOADLIBNAME, luaopen_package },
                                        //  { LUA_COLIBNAME, luaopen_coroutine },
                                        { LUA_TABLIBNAME, luaopen_table },
                                        { LUA_IOLIBNAME, luaopen_io },
                                        //  { LUA_OSLIBNAME, luaopen_os },
                                        { LUA_STRLIBNAME, luaopen_string },
                                        { LUA_MATHLIBNAME, luaopen_math },
                                        //  { LUA_UTF8LIBNAME, luaopen_utf8 },
                                        { LUA_DBLIBNAME, luaopen_debug },
#if defined( LUA_COMPAT_BITLIB )
                                        { LUA_BITLIBNAME, luaopen_bit32 },
#endif

                                        { NULL, NULL } };

static int luaPrint( lua_State* L ) {
    int nargs = lua_gettop( L );
    for( int i = 1; i <= nargs; ++i ) {
        REMOTE_LOGI(::esp_log_tags::LUA, lua_tostring( L, i ) );
    }
    return 0;
}

static const struct luaL_Reg printlib[] = { { "print", luaPrint }, { NULL, NULL } };

void loadDefaultLibs( lua_State* L ) {
    const luaL_Reg* lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for( lib = defaultLibs; lib->func; lib++ ) {
        luaL_requiref( L, lib->name, lib->func, 1 );
        lua_pop( L, 1 ); /* remove lib */
    }
}

}  // namespace

Executor::Executor(::gateways::IGatewayUserDataStorage& gatewayUserDataStorage,
                   const std::chrono::milliseconds& shutdownDelay,
                   const unsigned int msgLen )
: core::SingletonBaseImpl<Executor>( shutdownDelay, msgLen )
, m_scriptDebugMod( *this, "executor" )
, m_storageMod( *this, "storage", storage::BinaryFileStorageFactory::get() )
, m_zwaveMod( *this, common::resources::gateway_names::ZWave )
#ifdef CONFIG_CLOUD_DEVICES
, m_cloudMod( *this, common::resources::gateway_names::Cloud )
#endif
, m_coreMod( *this, "core" )
, m_timerMod( *this, "timer" )
#if CONFIG_REMOTE_LOGGING
, m_loggingMod( *this, "logging" )
#endif  // CONFIG_REMOTE_LOGGING
#if CONFIG_BOARD_TYPE_PLUG_HUB
, m_PlugHubMod( *this, "plughub" )
#endif
#if CONFIG_BOARD_TYPE_WATER_VALVE
, m_WaterValveMod( *this, "water_valve" )
#endif
, m_scenesMod( *this, "scenes" )
, m_gatewayUserDataStorage( gatewayUserDataStorage ) {
    LUA_LOGI( "lua::Executor: Instance created" );
}

Executor::~Executor() {
    LUA_LOGI( "lua::Executor: Instance deleted" );
    closeLuaInstance();
}

Executor::LuaInstGuard Executor::getOrCreateLuaInstance() {
    if( !m_luaCacheRevisionChecker() && m_lua ) {
        LUA_LOGD( "Plugins revision has been changed. Lua will be reseted." );
        closeLuaInstance();
    }

    return LuaInstGuard( m_lua ? m_lua : createLuaInstance(), LuaCleaner{ *this } );
}

lua_State* Executor::createLuaInstance() {
    assert( m_lua == nullptr );

    m_buffer.resize( BUFFER_SIZE );
#ifdef CONFIG_SPIRAM
    auto lua_alloc = []( void* ud, void* ptr, size_t osize, size_t nsize ) -> void* {
        (void)ud;    /* not used */
        (void)osize; /* not used */
        if( nsize == 0 ) {
            heap_caps_free( ptr );
            return NULL;
        }
        else {
            return heap_caps_realloc( ptr, nsize, MALLOC_CAP_32BIT | MALLOC_CAP_SPIRAM );
        }
    };
    m_lua = lua_newstate( lua_alloc, NULL );
#else   // not CONFIG_SPIRAM
    m_lua = luaL_newstate();
#endif  // CONFIG_SPIRAM

    lua_atpanic( m_lua, luaPanic );

    loadDefaultLibs( m_lua );

    lua_getglobal( m_lua, "_G" );
    luaL_setfuncs( m_lua, printlib, 0 );
    lua_pop( m_lua, 1 );

    regApi( m_lua );

    lua_newtable( m_lua );
    lua_setglobal( m_lua, luaDataKey );

    lua_pushstring( m_lua, "freertos" );
    lua_setglobal( m_lua, "_PLATFORM" );

    lua_pushinteger( m_lua,
#ifdef CONFIG_SPIRAM
                     2
#else
                     1
#endif
    );

    lua_setglobal( m_lua, "_HARDWARE_REVISION" );

    luaRet( 0 );

    return m_lua;
}

void Executor::closeLuaInstance() {
    if( m_lua ) {
        lua_close( m_lua );
        m_lua = nullptr;
        m_storageMod.releaseStorage();
    }
}

bool Executor::callReceiver( IModule* mod, const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommand& msg ) {
    const common::debug::DurationMeasurer durationMeasurer;

    auto lua_guard = getOrCreateLuaInstance();
    if( !lua_guard )
        return false;

    const auto luaStackSize = lua_gettop( m_lua );

    msg.accept( from, to, *mod );

    int retval = lua_pcall( m_lua, m_luaRet, 0, 0 );
    if( LUA_OK == retval ) {
        lua_guard.reset();

        LUA_LOGD( "Executor::call. time %lld ms", durationMeasurer.get().count() );
        return true;
    }

    REMOTE_LOGE( __FUNCTION__, "Error executing Lua script (call), return value: %d", retval );
    IModule::printLuaErrorDump( m_lua );

    lua_pop( m_lua, lua_gettop( m_lua ) - luaStackSize );

    return false;
}

std::tuple<bool, common::IValue::UPtr> Executor::execScript( const std::string& script,
                                                             std::function<int( lua_State* )> push_params_clbk ) {
    const common::debug::DurationMeasurer durationMeasurer;

    auto lua_guard = getOrCreateLuaInstance();

    if( !lua_guard ) {
        return std::make_tuple( false, nullptr );
    }

    const auto luaStackSize = lua_gettop( m_lua );

    if( !loadScript( script, true ) ) {
        lua_pop( m_lua, lua_gettop( m_lua ) - luaStackSize );
        return std::make_tuple( false, nullptr );
    }

    int n_params{ 0 };

    if( push_params_clbk ) {
        n_params = push_params_clbk( m_lua );
    }

    int status = lua_pcall( m_lua, n_params, LUA_MULTRET, 0 );
    if( LUA_OK != status ) {
        LUA_LOGE( "Executor::execScript: lua_pcall FAILED: status=%d, script=%s", status, script.c_str() );
        IModule::printLuaErrorDump( m_lua );
        lua_pop( m_lua, lua_gettop( m_lua ) - luaStackSize );
        return std::make_tuple( false, nullptr );
    }

    auto retval = IModule::createValueFromStack( m_lua );
    if( retval )
        lua_remove( m_lua, 1 );  // remove value from stack

    lua_guard.reset();

    LUA_LOGD( "Executor::execScript %s. time %lld ms", script.c_str(), durationMeasurer.get().count() );

    return std::make_tuple( true, std::move( retval ) );
}

void Executor::threadTask( std::shared_ptr<Executor> executor ) {
    LUA_LOGI( "Start lua::Executor::threadTask" );

    core::upd_srv::UpdContext ctx;
    while( core::upd_srv::ClbkRet::Completed != executor->update( ctx ) ) {
        ctx.yield();
    }

    LUA_LOGI( "Delete lua::Executor::threadTask" );
}

void Executor::LuaCleaner::operator()( lua_State* lua ) {
    m_owner.m_files.clear();  // TODO: Investigate, should we clear it for rev1 only
#ifndef CONFIG_SPIRAM
    m_owner.m_buffer.clear();
    m_owner.m_buffer.shrink_to_fit();

    m_owner.closeLuaInstance();
#endif
}

Executor::ClbkRet Executor::updateComponent( UpdContext& ctx ) {
    m_timerMod.update();
    return ClbkRet::Continue;
}

void Executor::regApi( lua_State* lua ) {
    lua_pushlightuserdata( lua, (void*)this );
    lua_pushcclosure( lua, Executor::luaRequire, 1 );
    lua_setglobal( lua, "require" );

    lua_pushlightuserdata( lua, (void*)this );
    lua_pushcclosure( lua, Executor::luaLoadfile, 1 );
    lua_setglobal( lua, "loadfile" );
}

int Executor::regModule( const char* name, lua_State* lua ) {
    if( name ) {
        for( auto& i : m_mods ) {
            if( i.name && !strcmp( i.name, name ) ) {
                return i.mod->regModule( lua );
            }
        }
    }
    return 0;
}

bool Executor::initializeMods() {
    bool isSuccess = true;
    for( auto& i : m_mods ) {
        if( i.mod->initialize() ) {
            LUA_LOGI( "lua::Executor: Initialize module %s successfully", i.name );
        }
        else {
            LUA_LOGE( "lua::Executor: Can't initialize module %s!", i.name );
            isSuccess = false;
        }
    }

    return isSuccess;
}

bool Executor::reg( IModule& mod, const char* name ) {
    auto i = std::find_if( m_mods.begin(), m_mods.end(), [&name, &mod]( Module& other ) -> bool {
        return other.mod == &mod && other.name == name;  /// INFO: optimazed name comparision
    } );
    if( i != m_mods.end() ) {
        LUA_LOGW( "lua::Executor: Try register previously registered module %s!", name );
        return true;  // #TODO: I think it should return false
    }

    LUA_LOGI( "lua::Executor: Lua module %s successfully registered", name );
    m_mods.emplace_back( name, mod );
    return true;
}

void Executor::receive( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommand& msg ) {
    for( auto& i : m_receivers ) {
        if( i.to == to && nullptr != i.mod ) {
            callReceiver( i.mod, from, to, msg );
        }
    }

    ParentType::receive( from, to, msg );
}

bool Executor::loadModuleFromCache( lua_State* lua, const char* name ) {
    if( !name ) {
        return false;
    }
    int type = lua_getglobal( lua, MODULE_CACHE_TABLE );
    if( type != LUA_TTABLE ) {
        lua_pop( lua, 1 );
        return false;
    }
    type = lua_getfield( lua, -1, name );
    if( type == LUA_TNIL ) {
        lua_pop( lua, 2 );
        return false;
    }
    lua_remove( lua, -2 );
    return true;
}

void Executor::storeModuleToCache( lua_State* lua, const char* name ) {
    int type = lua_getglobal( lua, MODULE_CACHE_TABLE );
    if( type != LUA_TTABLE ) {
        lua_pop( lua, 1 );
        lua_newtable( lua );
        lua_setglobal( lua, MODULE_CACHE_TABLE );
        type = lua_getglobal( lua, MODULE_CACHE_TABLE );
    }

    assert( type == LUA_TTABLE );
    lua_pushnil( lua );
    lua_copy( lua, -3, -1 );
    lua_setfield( lua, -2, name );
    lua_pop( lua, 1 );
    return;
}

int Executor::luaRequire( lua_State* lua ) {
    Executor& e = *( (Executor*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* name = luaL_checkstring( lua, 1 );
    if( !name ) {
        REMOTE_LOGW( __FUNCTION__, "Script's name is empty" );
        return 0;
    }

    if( loadModuleFromCache( lua, name ) ) {
        return 1;
    }
    int result = 0;
    std::string script_name( name );
    if( e.loadScript( script_name, false ) ) {
        int status = lua_pcall( e.lua(), 0, 1, 0 );
        if( LUA_OK != status ) {
            REMOTE_LOGE( __FUNCTION__, "Error executing Lua script (require)" );

            IModule::printLuaErrorDump( e.lua() );
            return 0;
        }

        storeModuleToCache( lua, script_name.c_str() );
        result = 1;
    }
    else {
        result = e.regModule( script_name.c_str(), lua );
        if( result )
            storeModuleToCache( lua, script_name.c_str() );
    }
    return result;
}

int Executor::luaLoadfile( lua_State* lua ) {
    Executor& e = *( (Executor*)lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
    const char* name = luaL_checkstring( lua, 1 );
    if( !name ) {
        REMOTE_LOGW( __FUNCTION__, "Script's name is empty" );
        return 0;
    }

    std::string script_name( name );
    int result{ 0 };
    if( e.loadScript( script_name, false ) ) {
        result = 1;
    }
    else {
#ifdef CONFIG_OPTIMIZATION_LEVEL_DEBUG
        REMOTE_LOGW( __FUNCTION__, "Can't loaded script: %s", script_name.c_str() );
#endif
    }
    return result;
}

const char* Executor::codeReader( lua_State* lua, void* ud, size_t* osize ) {
    if( !ud ) {
        *osize = 0;
        return nullptr;
    }
    Executor& self = *reinterpret_cast<Executor*>( ud );
    if( !self.m_currentFile->read( self.m_buffer.data(), self.m_buffer.size(), osize ) ) {
        *osize = 0;
        return nullptr;
    }
    return self.m_buffer.data();
}

bool Executor::busJoin( IModule& mod, const char* addrName, const char* file, const plugin::PluginInfo& plugin ) {
    if( !file ) {
        // For std::string initialization
        file = "";
    }

    bus::BusAddr addr = bus::Bus::get().nameToAddr( addrName );

    if( addr != bus::BusAddrInvalid ) {
        auto i = std::find_if(
            m_receivers.begin(), m_receivers.end(), [&addr, &mod, &plugin, &file]( auto& receiver ) -> bool {
                return ( receiver.to == addr ) && ( receiver.mod == &mod ) && ( receiver.plugin == plugin ) &&
                       ( receiver.file == file );
            } );
        if( i != m_receivers.end() ) {
            i->joinCount++;

            return true;
        }
    }

    if( bus::Bus::get().join( addrName, *this ) ) {
        addr = bus::Bus::get().nameToAddr( addrName );
        assert( addr != bus::BusAddrInvalid );

        auto i = std::find_if(
            m_receivers.begin(), m_receivers.end(), [&addr, &mod, &plugin, &file]( auto& receiver ) -> bool {
                return ( receiver.to == addr ) && ( receiver.mod == &mod ) && ( receiver.plugin == plugin ) &&
                       ( receiver.file == file );
            } );
        if( m_receivers.end() != i ) {
            LUA_LOGE( "Executor: Try to join already joined module!" );
            return false;
        }

        m_receivers.emplace_back( addr, mod, plugin, file );

        return true;
    }

    return false;
}

void Executor::busLeave( IModule& mod, const char* addrName, const char* file, const plugin::PluginInfo& plugin ) {
    const bus::BusAddr addr = bus::Bus::get().nameToAddr( addrName );

    auto i =
        std::find_if( m_receivers.begin(), m_receivers.end(), [&addr, &mod, &plugin, &file]( auto& receiver ) -> bool {
            return ( receiver.to == addr ) && ( receiver.mod == &mod ) && ( receiver.plugin == plugin ) &&
                   ( receiver.file == file );
        } );
    if( i != m_receivers.end() ) {
        if( i->joinCount ) {
            i->joinCount--;
        }
        else {
            bus::Bus::get().leave( addrName, *this );
            m_receivers.erase( i );
        }
    }
}

void Executor::handle( const bus::BusAddr from, const bus::BusAddr to, bus::command::HandleEventBase& command ) {
    const common::debug::DurationMeasurer durationMeasurer;

    ///////
#ifndef CONFIG_SPIRAM

    int i = 0;
    multi_heap_info_t info = {};
    while( i < 50 ) {
        heap_caps_get_info( &info, MALLOC_CAP_8BIT );
        if( info.total_free_bytes >= 15 * 1024 )
            break;
        vTaskDelay( pdMS_TO_TICKS( 100 ) );
        i++;
    }
    if( info.total_free_bytes < 15 * 1024 ) {
        REMOTE_LOGE( __FUNCTION__, " Not enough memory. Free memory - %d ", info.total_free_bytes );
    }
#endif

    auto lua_guard = getOrCreateLuaInstance();
    if( !lua_guard ) {
        return;
    }

    const auto luaStackSize = lua_gettop( m_lua );

    {
        if( command.handlerName().empty() ) {
            return;
        }

        if( !loadScript( command.handlerName(), true ) ) {
            return;
        }

        for( auto& i : m_mods ) {
            command.accept( from, to, *( i.mod ) );
        }
    }

    if( LUA_OK == lua_pcall( m_lua, m_luaRet, 0, 0 ) ) {
        lua_guard.reset();

        LUA_LOGD( "Executor::call. time %lld ms", durationMeasurer.get().count() );
        return;
    }

    REMOTE_LOGE( __FUNCTION__, "Error executing Lua script (call) handlerName=%s", command.handlerName().c_str() );
    IModule::printLuaErrorDump( m_lua );

    lua_pop( m_lua, lua_gettop( m_lua ) - luaStackSize );
}

void Executor::visit(::bus::BusAddr from, ::bus::BusAddr to, ::bus::command::SendGenericEventToLuaHandler& command ) {
    if( command.handlerName().empty() ) {
        return;
    }

    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from,
                      const bus::BusAddr to,
                      bus::command::HandleEventUpdateProductName& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::HubSettingsApply& command ) {
    for( auto& i : m_mods ) {
        command.accept( from, to, *( i.mod ) );
    }
}

void Executor::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueUpdated& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueSet& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetConsumedEnergyMeter& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from,
                      const bus::BusAddr to,
                      bus::command::ElectricMeterValuesUpdated& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ResetEnergyMeter& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::WaterLeakValueUpdate& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from,
                      const bus::BusAddr to,
                      cloud::responses::HandleEventCloudDeviceAdded& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from,
                      const bus::BusAddr to,
                      cloud::responses::HandleEventCloudDeviceRemoved& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from,
                      const bus::BusAddr to,
                      cloud::responses::HandleEventCloudDeviceUpdated& command ) {
    handle( from, to, command );
}

void Executor::visit( const bus::BusAddr from,
                      const bus::BusAddr to,
                      cloud::responses::HandleEventCloudResponseCommand& command ) {
    handle( from, to, command );
}

storage::ArchiveFile* Executor::getOrCreateArchiveFile( const std::string& archive_uri, const std::string& filename ) {
    if( !m_filesRevisionchecker() ) {
        m_files.clear();
    }

    auto tryOpenFile = [&]( storage::ArchiveFile* file ) -> storage::ArchiveFile* {
        if( !file->open( archive_uri.c_str(), filename.c_str() ) ) {
            return nullptr;
        }

        m_currentFile = file;
        return file;
    };

    auto found = m_files.find( archive_uri );

    if( found != m_files.end() ) {
        return tryOpenFile( found->second.get() );
    }

    auto file = storage::ArchiveFileFactory::getArchiveFile( archive_uri.c_str() );

    if( !file ) {
        LUA_LOGE( "Can't open archive %s", archive_uri.c_str() );
        return nullptr;
    }

    return tryOpenFile( m_files.emplace( archive_uri, std::move( file ) ).first->second.get() );
}

bool Executor::loadScript( const std::string& script_name, bool root ) {
    bool result{ false };
    auto plugin_id_begin = std::find( script_name.begin(), script_name.end(), ':' );
    if( plugin_id_begin == script_name.end() ) {
        plugin_id_begin = script_name.begin();
    }

    auto plugin_id_end = std::find( plugin_id_begin, script_name.end(), '/' );
    if( plugin_id_end != script_name.end() && ( plugin_id_end + 1 ) != script_name.end() ) {
        if( *plugin_id_begin == ':' ) {
            ++plugin_id_begin;
        }

        std::string plugin_id( plugin_id_begin, plugin_id_end );

        if( root ) {
            m_currentPluginID = plugin_id;
            IModule::setPluginId( m_lua, m_currentPluginID.c_str() );

            const auto luaStackSize = lua_gettop( m_lua );

            // trying to use caching loadfile for root scripts
            // file caching algorithm is platform/revision dependent and implemented in lua
            if( lua_getglobal( m_lua, "hubplugins" ) == LUA_TTABLE &&
                lua_getfield( m_lua, -1, "weak_loadfile" ) == LUA_TFUNCTION ) {
                lua_remove( m_lua, -2 );
                lua_pushstring( m_lua, script_name.c_str() );
                if( lua_pcall( m_lua, 1, 1, 0 ) != LUA_OK ) {
                    LUA_LOGE( "Cannot call script \"%s\" from cache.", script_name.c_str() );
                    IModule::printLuaErrorDump( m_lua );
                    lua_pop( m_lua, lua_gettop( m_lua ) - luaStackSize );
                    return false;
                }
                return true;
            }

            lua_pop( m_lua, lua_gettop( m_lua ) - luaStackSize );
        }

        std::string filename;
        static const std::string ext( ".lua" );
        filename.reserve( std::distance( plugin_id_end + 1, script_name.end() ) + ext.size() );
        filename.assign( plugin_id_end + 1, script_name.end() );
        if( filename.size() < ext.size() || filename.compare( filename.size() - ext.size(), ext.size(), ext ) != 0 ) {
            filename.append( ext );
        }

        const auto& plugins_list = plugin::PluginManager::get().getInstalledPluginList();
        const auto found_plugin =
            std::find_if( plugins_list.begin(), plugins_list.end(), [&plugin_id]( const auto& plugin ) {
                return plugin.plugin.id == plugin_id;
            } );

        if( found_plugin == plugins_list.end() ) {
            return false;
        }

        if( !getOrCreateArchiveFile( found_plugin->uri, filename ) ) {
            return false;
        }

#ifndef CONFIG_SPIRAM
        // execute full garbage collection cycle
        lua_gc( m_lua, LUA_GCCOLLECT, 0 );
#endif
        result = LUA_OK == lua_load( m_lua, &codeReader, this, filename.c_str(), nullptr );
        if( !result ) {
            LUA_LOGE( "Cannot load script \"%s\"", filename.c_str() );
            IModule::printLuaErrorDump( m_lua );
        }
        m_currentFile = nullptr;
    }
    return result;
}

}  // namespace lua
