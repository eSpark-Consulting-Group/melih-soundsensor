#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "plugin_manager.h"
#include "StorageArchiveFileFactory.h"

#include <list>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onExtensionsPluginInstallExecute( const gason::JsonValue& json,
                                                                                   common::JsonStreamGenerator& gen ) {
    std::list<plugin::PluginURI> pluginURIs;
    for( auto& it : json( tokens::Params )( tokens::Plugins ) ) {
        plugin::PluginURI plugUri;
        const char* id = it.value( tokens::ExtensionID ).toString();
        const char* ver = it.value( tokens::ExtensionVersion ).toString();
        const char* uri = it.value( tokens::PluginURI ).toString();
        if( id && ver && uri &&
            sscanf( ver, "%hhu.%hhu", &plugUri.plugin.version.major, &plugUri.plugin.version.minor ) == 2 ) {
            plugUri.plugin.id = id;
            plugUri.size = it.value( tokens::PluginSize ).toInt();
            plugUri.uri = uri;
            plugUri.format =
                storage::ArchiveFileFactory::getFileFormatFromName( it.value( tokens::PluginFormat ).toString() );

            pluginURIs.emplace_back( std::move( plugUri ) );
        }
    }

    if( !( gen.writeOpeningObject() && gen.writeClosingSymbol() ) ) {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }

    auto ret = plugin::PluginManager::get().doInstall( gen, std::move( pluginURIs ) );
    if( plugin::PluginManager::Result::OK == ret ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
}

}  // namespace nma
