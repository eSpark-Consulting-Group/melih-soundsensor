#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesSwitchCancel( const gason::JsonValue& json,
                                                                         common::JsonStreamGenerator& gen ) {
    bool result = ( core::HouseModesManager::Result::OK == core::HouseModesManager::get().cancelSwitchPreset( gen ) );
    if( !result ) {
        gen.writeEmptyObject() && gen.writeItemDelimiter();
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma