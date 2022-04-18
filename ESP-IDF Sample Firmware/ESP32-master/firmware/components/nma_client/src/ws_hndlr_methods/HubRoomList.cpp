#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "PagesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubRoomList( const gason::JsonValue& json,
                                                                common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    bool result = true;
    result = result && jsGen.writeOpeningArray();
    if( result ) {
        auto pagesManager = core::PagesManager::get();
        result = pagesManager && pagesManager->listPages( jsGen );
    }
    result = result && jsGen.writeClosingSymbol();
    if( !result ) {
        errorCode = errors::ApplicationError;
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma