#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "PagesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubRoomAllDelete( const gason::JsonValue& json,
                                                                     common::JsonStreamGenerator& jsGen ) {
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    auto pagesManager = core::PagesManager::get();
    if( pagesManager && pagesManager->deleteAllPages() ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    return std::make_tuple( errors::SystemError, StringType() );
}

}  // namespace nma