#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "PagesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubRoomNameSet( const gason::JsonValue& json,
                                                                   common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();

    auto GetParamsTokenElement = json( tokens::Params );

    core::PagesManager::PageId roomId = 0;
    auto roomIdRawToken = GetParamsTokenElement( tokens::SubId ).toString();
    if( roomIdRawToken ) {
        sscanf( (char*)roomIdRawToken, "%" PRIX32, &roomId );
    }

    char* name = GetParamsTokenElement( tokens::Name ).toString();

    auto pagesManager = core::PagesManager::get();
    if( !name || !pagesManager || !pagesManager->setPageName( roomId, name ) ) {
        errorCode = errors::InvalidParams;
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma