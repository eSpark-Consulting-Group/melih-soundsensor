#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "PagesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubRoomGet( const gason::JsonValue& json,
                                                               common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    auto GetParamsTokenElement = json( tokens::Params );

    core::PagesManager::PageId roomId = 0;
    auto roomIdRawToken = GetParamsTokenElement( tokens::SubId ).toString();
    if( roomIdRawToken ) {
        sscanf( (char*)roomIdRawToken, "%" PRIX32, &roomId );
    }

    bool result = true;
    result = result && jsGen.writeOpeningArray();
    if( result ) {
        auto pagesManager = core::PagesManager::get();
        result = pagesManager && pagesManager->listPage( jsGen, roomId );
    }
    result = result && jsGen.writeClosingSymbol();

    if( !result ) {
        errorCode = errors::ApplicationError;
    }

    return std::make_tuple( errorCode, StringType() );
}

}  // namespace nma