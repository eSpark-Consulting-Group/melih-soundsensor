
#include "LuaModuleScriptDebug.h"

#include <algorithm>

#include "Lua.h"
#include "BusCommandPluginRun.h"
#include <BusCommandCloudForceRemove.h>
#include <BusCommandCloudItemValueSet.h>
#include <BusCommandCloudDictionaryValue.h>
#include <BusCommandHubDeviceFirmwareUpdate.h>
#include "settings/commands/BusCommandDeviceSettingDictionaryValueSet.h"
#include "settings/commands/BusCommandDeviceSettingValueSet.h"
#include "settings/commands/BusCommandGatewaySettingValueSet.h"
#include "settings/commands/BusCommandDeviceSettingReset.h"
#include <CommonResources.h>
#include <DebugUtils.h>
#include <Utils.h>
#include <GatewayUtils.h>
#include <plugin_manager.h>
#include <plugin_info.h>
#include <StorageArchiveFileFactory.h>
#include <mbedtls/base64.h>
#include <Cache.h>

#include "converters/ValueToLuaGenerator.h"

#include "IValue.h"
#include "IValueCast.h"
#include "values/ArrayValue.h"
#include "values/TableValue.h"
#include "values/StringValue.h"

namespace lua {
namespace module {

using namespace common::resources;

constexpr char ScriptDebug::CHUNK_NAME[];

ScriptDebug::ScriptDebug( Executor& exec, const char* name )
: IModule( exec, name ) {}

bool ScriptDebug::initialize() {
    bool result = m_exec.busJoin( *this, "debug" );
    return result;
}

void ScriptDebug::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::PluginRun& command ) {
    if( !command.scriptCode.empty() ) {
        const size_t scriptCodeSize = command.scriptCode.length();
        size_t decoded_size{ 0 };
        mbedtls_base64_decode(
            nullptr, 0, &decoded_size, (const unsigned char*)command.scriptCode.c_str(), scriptCodeSize );
        std::vector<char> decoded_code( decoded_size, 0 );
        auto decoded_code_buf = reinterpret_cast<unsigned char*>( decoded_code.data() );
        mbedtls_base64_decode( decoded_code_buf,
                               decoded_size,
                               &decoded_size,
                               (const unsigned char*)command.scriptCode.c_str(),
                               scriptCodeSize );

        if( LUA_OK != luaL_loadbuffer( m_exec.lua(), decoded_code.data(), decoded_code.size(), CHUNK_NAME ) ) {
            FUNC_LOGE( "Can't load Lua script" );
        }
    }
    else if( !command.scriptName.empty() ) {
        readScriptFromArchive( command.scriptName );
    }

    if( command.scriptParams ) {
        ValueToLuaGenerator generator( m_exec.lua() );
        command.scriptParams->accept( generator );
        m_exec.luaRet( 1 );
    }
    else {
        m_exec.luaRet( 0 );
    }
}

void ScriptDebug::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::CloudItemValueSet& command ) {
    m_exec.luaRet( 0 );

    auto pConf = plugin::PluginManager::get().getPluginConfig( command.pluginName );
    auto scriptName = pConf.json( "gateway" )( "setItemValueCommand" ).toString();
    if( !readScriptFromArchive( scriptName ) ) {
        return;
    }

    // Clear stored state if any for proper operation result handling
    for( const auto& item : command.items ) {
        cache::Cache::get().setExpired( item.c_str() );
    }

    lua_createtable( m_exec.lua(), 0, 2 );

    // add "item_id" to table
    if( command.items.size() == 1 ) {
        lua_pushstring( m_exec.lua(), command.items.begin()->c_str() );
        lua_setfield( m_exec.lua(), -2, "item_id" );
    }
    else {
        lua_newtable( m_exec.lua() );
        unsigned int idx = 0;
        for( const auto& item : command.items ) {
            lua_pushnumber( m_exec.lua(), ++idx );
            lua_pushstring( m_exec.lua(), item.c_str() );
            lua_settable( m_exec.lua(), -3 );
        }
        lua_setfield( m_exec.lua(), -2, "item_ids" );
    }

    if( command.value ) {
        // add "value" to table
        ValueToLuaGenerator generator( m_exec.lua() );
        command.value->accept( generator );
        lua_setfield( m_exec.lua(), -2, common::resources::tokens::Value );
        m_exec.luaRet( 1 );
    }
}

void ScriptDebug::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::CloudDictionaryValue& command ) {
    m_exec.luaRet( 0 );

    auto pConf = plugin::PluginManager::get().getPluginConfig( command.pluginName() );

    std::string scriptName;
    switch( command.command() ) {
    case bus::command::CloudDictionaryValue::Add:
        scriptName = pConf.json( "gateway" )( "addItemDictionaryValueCommand" ).toString();
        break;
    case bus::command::CloudDictionaryValue::Set:
        scriptName = pConf.json( "gateway" )( "setItemDictionaryValueCommand" ).toString();
        break;
    case bus::command::CloudDictionaryValue::Remove:
        scriptName = pConf.json( "gateway" )( "removeItemDictionaryValueCommand" ).toString();
        break;
    case bus::command::CloudDictionaryValue::Clear:
        scriptName = pConf.json( "gateway" )( "clearItemDictionaryCommand" ).toString();
        break;
    }
    if( readScriptFromArchive( scriptName ) ) {
        // Clear stored state if any for proper operation result handling
        cache::Cache::get().setExpired( command.itemId().c_str() );

        lua_createtable( m_exec.lua(), 0, 2 );

        // add "item_id" to table
        lua_pushstring( m_exec.lua(), command.itemId().c_str() );
        lua_setfield( m_exec.lua(), -2, "item_id" );

        // add "key" to table
        if( !command.key().empty() ) {
            lua_pushstring( m_exec.lua(), command.key().c_str() );
            lua_setfield( m_exec.lua(), -2, common::resources::tokens::Key );
        }

        if( command.command() == bus::command::CloudDictionaryValue::Set ||
            command.command() == bus::command::CloudDictionaryValue::Add ) {
            // add "value_type" to table
            lua_pushstring( m_exec.lua(), command.valueType().c_str() );
            lua_setfield( m_exec.lua(), -2, "value_type" );

            // add "value" to table
            ValueToLuaGenerator generator( m_exec.lua() );
            command.value()->accept( generator );
            lua_setfield( m_exec.lua(), -2, common::resources::tokens::Value );
        }
        m_exec.luaRet( 1 );
    }
}

void ScriptDebug::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::CloudForceRemove& command ) {
    m_exec.luaRet( 0 );
    auto pluginName = gateways::utils::getPluginNameByDeviceId( command.deviceId.c_str() );
    if( pluginName.empty() ) {
        return;
    }

    auto pConf = plugin::PluginManager::get().getPluginConfig( pluginName );
    std::string scriptName = pConf.json( "gateway" )( "forceRemoveDeviceCommand" ).toString();

    if( readScriptFromArchive( scriptName ) ) {
        lua_newtable( m_exec.lua() );
        lua_pushstring( m_exec.lua(), command.deviceId.c_str() );
        lua_setfield( m_exec.lua(), -2, "deviceId" );
        m_exec.luaRet( 1 );
    }
}

void ScriptDebug::visit( const bus::BusAddr from,
                         const bus::BusAddr to,
                         settings::commands::GatewaySettingValueSet& command ) {
    m_exec.luaRet( 0 );
    auto pluginName = command.settingId();
    pluginName = pluginName.substr( 0, pluginName.find( "." ) );
    if( pluginName.empty() ) {
        return;
    }

    auto pConf = plugin::PluginManager::get().getPluginConfig( pluginName );

    std::string scriptName = pConf.json( "gateway" )( "setGatewaySettingValueCommand" ).toString();
    if( readScriptFromArchive( scriptName ) ) {
        lua_createtable( m_exec.lua(), 0, 2 );

        // add "setting_id" to table
        lua_pushstring( m_exec.lua(), command.settingId().c_str() );
        lua_setfield( m_exec.lua(), -2, "setting_id" );

        // add "value" to table
        if( command.value() ) {
            ValueToLuaGenerator generator( m_exec.lua() );
            command.value()->accept( generator );
            lua_setfield( m_exec.lua(), -2, common::resources::tokens::Value );
        }
        m_exec.luaRet( 1 );
    }
}

void ScriptDebug::visit( const bus::BusAddr from,
                         const bus::BusAddr to,
                         settings::commands::DeviceSettingValueSet& command ) {
    m_exec.luaRet( 0 );
    auto pluginName = gateways::utils::getPluginNameBySettingId( command.settingId().c_str() );
    if( pluginName.empty() ) {
        return;
    }

    auto pConf = plugin::PluginManager::get().getPluginConfig( pluginName );

    std::string scriptName = pConf.json( "gateway" )( "setSettingValueCommand" ).toString();
    if( readScriptFromArchive( scriptName ) ) {
        lua_createtable( m_exec.lua(), 0, 2 );

        // add "setting_id" to table
        lua_pushstring( m_exec.lua(), command.settingId().c_str() );
        lua_setfield( m_exec.lua(), -2, "setting_id" );

        // add "value" to table
        if( command.value() ) {
            ValueToLuaGenerator generator( m_exec.lua() );
            command.value()->accept( generator );
            lua_setfield( m_exec.lua(), -2, common::resources::tokens::Value );
        }
        m_exec.luaRet( 1 );
    }
}

void ScriptDebug::visit( const bus::BusAddr from,
                         const bus::BusAddr to,
                         settings::commands::DeviceSettingReset& command ) {
    m_exec.luaRet( 0 );
    std::string pluginName;
    bool bySettinId = false;
    const auto settingId = command.settingId();
    if( !settingId.empty() ) {
        pluginName = gateways::utils::getPluginNameBySettingId( command.settingId().c_str() );
        bySettinId = true;
    }
    else {
        pluginName = gateways::utils::getPluginNameByDeviceId( command.deviceId().c_str() );
    }

    if( pluginName.empty() ) {
        return;
    }

    auto pConf = plugin::PluginManager::get().getPluginConfig( pluginName );

    std::string scriptName = pConf.json( "gateway" )( "resetSettingCommand" ).toString();
    if( readScriptFromArchive( scriptName ) ) {
        lua_createtable( m_exec.lua(), 0, 2 );

        if( bySettinId ) {
            // add "setting_id" to table
            lua_pushstring( m_exec.lua(), command.settingId().c_str() );
            lua_setfield( m_exec.lua(), -2, "setting_id" );
        }
        else {
            // add "device_id" to table
            lua_pushstring( m_exec.lua(), command.deviceId().c_str() );
            lua_setfield( m_exec.lua(), -2, "device_id" );
        }

        m_exec.luaRet( 1 );
    }
}

void ScriptDebug::visit( const bus::BusAddr from,
                         const bus::BusAddr to,
                         settings::commands::DeviceSettingDictionaryValueSet& command ) {
    m_exec.luaRet( 0 );

    auto pluginName = gateways::utils::getPluginNameBySettingId( command.settingId().c_str() );
    if( pluginName.empty() ) {
        FUNC_LOGW( "Can't get a plugin's name for settingId=%s", command.settingId().c_str() );
        return;
    }

    auto pConf = plugin::PluginManager::get().getPluginConfig( pluginName );

    std::string scriptName = pConf.json( "gateway" )( "setSettingDictionaryValueCommand" ).toString();
    if( readScriptFromArchive( scriptName ) ) {
        lua_createtable( m_exec.lua(), 0, 2 );

        // add "setting_id" to table
        lua_pushstring( m_exec.lua(), command.settingId().c_str() );
        lua_setfield( m_exec.lua(), -2, "setting_id" );

        // add "setting_id" to table
        lua_pushstring( m_exec.lua(), command.key().c_str() );
        lua_setfield( m_exec.lua(), -2, "key" );

        // add "value" to table
        if( command.value() ) {
            ValueToLuaGenerator generator( m_exec.lua() );
            command.value()->accept( generator );
            lua_setfield( m_exec.lua(), -2, common::resources::tokens::Value );
        }

        m_exec.luaRet( 1 );
    }
}

bool ScriptDebug::readScriptFromArchive( std::string script_name ) {
    return m_exec.loadScript( script_name, true );
}

void ScriptDebug::visit( const bus::BusAddr from,
                         const bus::BusAddr to,
                         bus::command::HubDeviceFirmwareUpdate& command ) {
    m_exec.luaRet( 0 );

    auto arrayUpdates = common::iValueCast<common::ArrayValue>( command.updates().get() );
    for( auto& field : *arrayUpdates ) {
        auto table = common::iValueCast<common::TableValue>( field.get() );
        auto arrayDevices = common::iValueCast<common::ArrayValue>( ( *table )[common::resources::tokens::Devices] );
        auto deviceId = common::iValueCast<common::StringValue>( ( *arrayDevices->begin() ).get() )->get();

        auto pluginName = gateways::utils::getPluginNameByDeviceId( deviceId.c_str() );
        if( pluginName.empty() ) {
            FUNC_LOGW( "Can't get a plugin's name for deviceId=%s", deviceId.c_str() );
            return;
        }

        auto pConf = plugin::PluginManager::get().getPluginConfig( pluginName );
        std::string scriptName = pConf.json( "gateway" )( "updateDeviceFirmwareCommand" ).toString();
        if( readScriptFromArchive( scriptName ) ) {
            lua_createtable( m_exec.lua(), 0, 2 );

            ValueToLuaGenerator generator( m_exec.lua() );
            arrayDevices->accept( generator );
            lua_setfield( m_exec.lua(), -2, common::resources::tokens::Devices );

            auto arrayFirmware =
                common::iValueCast<common::ArrayValue>( ( *table )[common::resources::tokens::Firmware] );
            arrayFirmware->accept( generator );
            lua_setfield( m_exec.lua(), -2, common::resources::tokens::Firmware );

            m_exec.luaRet( 1 );
        }
    }
}

}  // namespace module
}  // namespace lua
