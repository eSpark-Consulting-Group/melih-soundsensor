#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "HouseModesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubModesCurrentGet( const gason::JsonValue& json,
                                                                       common::JsonStreamGenerator& jsGen ) {
    bool result = ( core::HouseModesManager::Result::OK == core::HouseModesManager::get().getCurrent( jsGen ) );
    if( !result ) {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }

    return std::make_tuple( errors::NotError, StringType() );
}

}  // namespace nma