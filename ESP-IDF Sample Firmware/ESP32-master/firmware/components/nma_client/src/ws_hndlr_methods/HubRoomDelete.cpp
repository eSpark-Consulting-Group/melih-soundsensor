#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "PagesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubRoomDelete( const gason::JsonValue& json,
                                                                  common::JsonStreamGenerator& jsGen ) {
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    auto GetParamsTokenElement = json( tokens::Params );

    core::PagesManager::PageId roomId = 0;
    auto roomIdRawToken = GetParamsTokenElement( tokens::SubId ).toString();
    if( roomIdRawToken ) {
        sscanf( (char*)roomIdRawToken, "%" PRIX32, &roomId );
    }

    auto pagesManager = core::PagesManager::get();
    if( pagesManager && pagesManager->deletePage( roomId ) ) {
        return std::make_tuple( errors::NotError, StringType() );
    }

    return std::make_tuple( errors::InvalidParams, StringType() );
}

}  // namespace nma