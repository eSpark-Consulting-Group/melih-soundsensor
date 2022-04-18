#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "PagesManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubRoomCreate( const gason::JsonValue& json,
                                                                  common::JsonStreamGenerator& jsGen ) {
    auto errorCode = errors::NotError;
    StringType errorMessage;
    bool isOk = false;
    auto name = json( tokens::Params )( tokens::Name ).toString();
    auto pagesManager = core::PagesManager::get();
    isOk = ( nullptr != name ) && bool( pagesManager );
    if( isOk ) {
        core::PagesManager::PageId pageId = 0;
        std::tie( errorCode, errorMessage ) = pagesManager->createPage( name, pageId );
        isOk = errors::NotError == errorCode;
        if( isOk ) {
            isOk = jsGen.writeOpeningObject();
            {
                std::array<char, 64> buffer;
                snprintf( buffer.data(), buffer.size(), "%" PRIX32, pageId );
                isOk = isOk && jsGen.writeProperty( tokens::SubId, buffer.data() );
            }
            isOk = isOk && jsGen.writeClosingSymbol();

            if( !isOk ) {
                errorCode = errors::InternalError;
            }
        }
    }
    else {
        errorCode = errors::InvalidRequest;
        errorMessage = errors::data::RpcParamsEmptyName;
    }

    if( !isOk ) {
        jsGen.writeEmptyObject() && jsGen.writeItemDelimiter();
    }

    return std::make_tuple( errorCode, std::move( errorMessage ) );
}

}  // namespace nma