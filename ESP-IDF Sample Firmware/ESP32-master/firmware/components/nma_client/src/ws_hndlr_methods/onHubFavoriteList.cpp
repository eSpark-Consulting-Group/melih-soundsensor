#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "FavoriteManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubFavoriteList( const gason::JsonValue& json,
                                                                    common::JsonStreamGenerator& gen ) {
    bool isValidRequest = json( tokens::Params )( tokens::Request ).at( 0 );
    if( !isValidRequest ) {
        gen.writeEmptyObject() && gen.writeItemDelimiter();
        return std::make_tuple( errors::InvalidParams, "*request* doesn't exist, have invalid type or empty" );
    }

    bool isSuccess = gen.writeOpeningObject();
    if( isSuccess ) {
        isSuccess = gen.writePropertyKey( tokens::Favorites );
        isSuccess = isSuccess && gen.writeOpeningObject();
        {
            auto getRequestTokenElement = json( tokens::Params )( tokens::Request );

            if( getRequestTokenElement ) {
                for( size_t typeIt = 0; getRequestTokenElement[typeIt]; ++typeIt ) {
                    char* favoriteTypeName = getRequestTokenElement[typeIt].toString();
                    isSuccess = isSuccess && core::FavoriteManager::getFavoriteList( gen, favoriteTypeName );
                }
            }
        }
        isSuccess = isSuccess && gen.writeClosingSymbol();
    }
    isSuccess = isSuccess && gen.writeClosingSymbol();

    if( isSuccess ) {
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
}

}  // namespace nma