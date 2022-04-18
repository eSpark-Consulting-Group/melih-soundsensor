#include "NmaWebSocketHandler.h"

#include "JsonStreamGenerator.h"
#include "CommonResources.h"
#include "FirmwareVersion.h"
#include "plugin_manager.h"
#include "znet_release_info.h"

#include "DebugUtils.h"

#include <tuple>
#include <string>
#include <cstdio>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubSoftwareInfoGet( const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& gen ) {
    bool isSuccess = gen.writeOpeningObject();
    {
        isSuccess = isSuccess && gen.writeProperty( tokens::Firmware, FIRMWARE_VERSION );

        isSuccess = isSuccess && gen.writePropertyKey( common::resources::tokens::Addons );
        isSuccess = isSuccess && gen.writeOpeningArray();
        {
            isSuccess = isSuccess && gen.writeOpeningObject();
            isSuccess = isSuccess && gen.writeProperty( common::resources::tokens::ExtensionID, "zwave" );
            char versionCStr[32];
            if( isSuccess && 0 < znetVersion( versionCStr, sizeof( versionCStr ) ) ) {
                isSuccess = gen.writeProperty( common::resources::tokens::ExtensionVersion, versionCStr );
            }
            isSuccess = isSuccess && gen.writeClosingSymbol();

            isSuccess = isSuccess && gen.writeOpeningObject();
            isSuccess = isSuccess && gen.writeProperty( common::resources::tokens::ExtensionID, "lua" );
            isSuccess = isSuccess && gen.writeProperty( common::resources::tokens::ExtensionVersion, FIRMWARE_VERSION );
            isSuccess = isSuccess && gen.writeClosingSymbol();
        }
        isSuccess = isSuccess && gen.writeClosingSymbol();  // Addons array

        isSuccess = isSuccess && gen.writePropertyKey( common::resources::tokens::Plugins );
        isSuccess = isSuccess && gen.writeOpeningArray();
        {
            auto& pman = plugin::PluginManager::get();
            isSuccess = isSuccess && pman.updateInstalledPluginList();
            const auto& plugins = pman.getInstalledPluginList();
            for( const auto& plug : plugins ) {
                isSuccess = isSuccess && gen.writeOpeningObject();
                isSuccess = isSuccess && gen.writeProperty( tokens::ExtensionID, plug.plugin.id );
                auto config = pman.getPluginConfig( plug.uri, plug.format );
                if( config.getId() == plug.plugin.id.c_str() ) {
                    auto verCStr = config.json( tokens::ExtensionVersion ).toString();
                    if( isSuccess && verCStr ) {
                        isSuccess = gen.writeProperty( tokens::ExtensionVersion, verCStr );
                    }
                }
                isSuccess = isSuccess && gen.writeClosingSymbol();
            }
        }
        isSuccess = isSuccess && gen.writeClosingSymbol();  // Plugin array
    }
    isSuccess = isSuccess && gen.writeClosingSymbol();

    return std::make_tuple( ( isSuccess ? errors::NotError : errors::ApplicationError ), "" );
}

}  // namespace nma
