#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesGet( const gason::JsonValue& json,
                                                                common::JsonStreamGenerator& jsGen ) {
    bool isSuccess = ( core::HouseModesManager::Result::OK == core::HouseModesManager::get().getPresets( jsGen ) );
    if( !isSuccess ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
        return std::make_tuple( errors::ApplicationError, StringType() );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma