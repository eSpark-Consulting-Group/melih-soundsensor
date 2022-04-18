#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "plugin_manager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onExtensionsPluginInfoInstalled( const gason::JsonValue& json,
                                                                                  common::JsonStreamGenerator& gen ) {
    if( plugin::PluginManager::Result::OK == plugin::PluginManager::get().getInstalled( gen ) ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
}

}  // namespace nma