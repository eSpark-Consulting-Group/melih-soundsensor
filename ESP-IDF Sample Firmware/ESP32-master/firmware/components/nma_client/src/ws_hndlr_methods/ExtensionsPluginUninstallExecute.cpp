#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "plugin_manager.h"

#include <list>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onExtensionsPluginUninstallExecute(
    const gason::JsonValue& json,
    common::JsonStreamGenerator& gen ) {
    const char* plugId = json( tokens::Params )( tokens::Plugin )( tokens::ExtensionID ).toString();
    const char* plugVer = json( tokens::Params )( tokens::Plugin )( tokens::ExtensionVersion ).toString();
    if( !( gen.writeOpeningObject() && gen.writeClosingSymbol() ) ) {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
    auto ret = plugin::PluginManager::get().doUninstall( gen, plugId, plugVer );
    bool result = ( plugin::PluginManager::Result::OK == ret );
    if( result ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
}

}  // namespace nma
