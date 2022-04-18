#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "plugin_manager.h"
#include "StorageArchiveFileFactory.h"

#include <list>

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onExtensionsPluginInstallRequest( const gason::JsonValue& json,
                                                                                   common::JsonStreamGenerator& gen ) {
    bool isSuccess = gen.writeOpeningObject();
    if( isSuccess ) {
        auto ret = plugin::PluginManager::get().getAll( gen );
        isSuccess = ( plugin::PluginManager::Result::OK == ret );
    }
    isSuccess = isSuccess && gen.writeClosingSymbol();
    if( isSuccess ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
}

}  // namespace nma