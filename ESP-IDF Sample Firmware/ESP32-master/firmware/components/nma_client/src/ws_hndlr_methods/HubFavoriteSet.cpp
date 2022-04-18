#include "NmaWebSocketHandler.h"
#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "CommonResources.h"

#include "FavoriteManager.h"

namespace nma {

using namespace common::resources;

WebSocketHandler::ResponceType WebSocketHandler::onHubFavoriteSet( const gason::JsonValue& json,
                                                                   common::JsonStreamGenerator& gen ) {
    gen.writeEmptyObject() && gen.writeItemDelimiter();

    auto params = json( tokens::Params );

    {
        auto devicesId = params( tokens::Devices ).at( 0 );
        auto itemsId = params( tokens::Items ).at( 0 );
        auto rulesId = params( tokens::Rules ).at( 0 );

        if( devicesId && itemsId && rulesId ) {
            return std::make_tuple( errors::InvalidParams, "*params* doesn't exist, have invalid type or empty" );
        }
    }

    bool isSuccess = true;
    for( size_t i = 0; i < core::FavoriteManager::favoriteTypesCount; ++i ) {
        auto GetFavoriteTypeNameData = params( core::FavoriteManager::favoriteTypes[i] );

        if( GetFavoriteTypeNameData ) {
            const char* favoriteTypeName = core::FavoriteManager::favoriteTypes[i];

            for( size_t favoriteTypeNameDataIt = 0; GetFavoriteTypeNameData[favoriteTypeNameDataIt];
                 ++favoriteTypeNameDataIt ) {
                char* id =
                    GetFavoriteTypeNameData[favoriteTypeNameDataIt]( common::resources::tokens::SubId ).toString();
                bool isSet =
                    GetFavoriteTypeNameData[favoriteTypeNameDataIt]( common::resources::tokens::Favorite ).toBool();

                isSuccess = core::FavoriteManager::updateFavoriteId( favoriteTypeName, id, isSet );
                if( isSuccess ) {
                    break;
                }
            }

            if( !isSuccess ) {
                break;
            }
        }
    }

    if( isSuccess ) {
        core::FavoriteManager::updateFavoriteIdBroadcast( m_client );
        return std::make_tuple( errors::NotError, StringType() );
    }
    else {
        return std::make_tuple( errors::ApplicationError, StringType() );
    }
}

}  // namespace nma