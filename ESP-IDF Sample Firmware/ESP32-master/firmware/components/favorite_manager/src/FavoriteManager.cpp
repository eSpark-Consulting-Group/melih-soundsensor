#include <FavoriteManager.h>
#include <JsonStreamGenerator.h>
#include <CommonResources.h>

#include <DebugUtils.h>
#include <Utils.h>
#include <string.h>

#include <NmaBroadcastsSender.h>

#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>
#include <BusCommandFavoriteUpdated.h>

namespace core {

namespace FavoriteManager {

const char* favoriteTypes[TypeEnd] = { common::resources::tokens::Devices,
                                       common::resources::tokens::Items,
                                       common::resources::tokens::Rules };

namespace {
FavoriteTypesListBroadcast m_addIdObject;
FavoriteTypesListBroadcast m_removeIdObject;
}  // namespace

bool getFavoriteList( common::JsonStreamGenerator& result, const char* favoriteTypeName ) {
    auto favoriteType = getType( favoriteTypeName );
    const char* section;
    switch( favoriteType ) {
    case Device:
        section = storage::section::FAV_DEVS;
        break;
    case Item:
        section = storage::section::FAV_ITEMS;
        break;
    case Rule:
        section = storage::section::FAV_RULES;
        break;
    default:
        return false;
    }
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( section );
    if( !stor )
        return false;

    bool res = true;
    res = res && result.writePropertyKey( favoriteTypes[favoriteType] );
    res = res && result.writeOpeningArray();

    if( res ) {
        auto it = stor->cbegin(), end = stor->cend();
        for( ; it != end; ++it ) {
            res = res && result.writeProperty( NULL, ( *it ).c_str() );
            if( !res )
                break;
        }
    }

    res = res && result.writeClosingSymbol();

    return res;
}

bool updateFavoriteId( const char* favoriteTypeName, const char* favoriteId, const bool isAdd ) {
    auto favoriteType = getType( favoriteTypeName );
    const char* section;
    switch( favoriteType ) {
    case Device:
        section = storage::section::FAV_DEVS;
        break;
    case Item:
        section = storage::section::FAV_ITEMS;
        break;
    case Rule:
        section = storage::section::FAV_RULES;
        break;
    default:
        return false;
    }
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( section );
    if( !stor )
        return false;

    bool res = true;
    if( isAdd ) {
        res = stor->setInt8( favoriteId, 1 );
        m_addIdObject[favoriteTypeName].push_back( favoriteId );
    }
    else {
        stor->remove( favoriteId );
        m_removeIdObject[favoriteTypeName].push_back( favoriteId );
    }

    return res;
}

void updateFavoriteIdBroadcast( bus::IBusClient& client ) {
    if( !m_addIdObject.empty() ) {
        auto favoriteUpdated = std::make_unique<bus::command::FavoriteUpdated>();
        favoriteUpdated->m_operation = bus::command::FavoriteUpdated::Operation::added;
        favoriteUpdated->m_favoriteObject = m_addIdObject;
        client.send( "nma", std::move( favoriteUpdated ) );
        m_addIdObject.clear();
    }

    if( !m_removeIdObject.empty() ) {
        auto favoriteUpdated = std::make_unique<bus::command::FavoriteUpdated>();
        favoriteUpdated->m_operation = bus::command::FavoriteUpdated::Operation::removed;
        favoriteUpdated->m_favoriteObject = m_removeIdObject;
        client.send( "nma", std::move( favoriteUpdated ) );
        m_removeIdObject.clear();
    }
}

FavoriteType getType( const char* favoriteTypeName ) {
    if( !( favoriteTypeName ) ) {
        FUNC_LOGI( "Debug: getType: type is empty" );
        return FavoriteType::TypeEnd;
    }

    if( strncmp( favoriteTypeName,
                 favoriteTypes[FavoriteType::Device],
                 strlen( favoriteTypes[FavoriteType::Device] ) ) == 0 ) {
        return FavoriteType::Device;
    }
    else if( strncmp( favoriteTypeName,
                      favoriteTypes[FavoriteType::Item],
                      strlen( favoriteTypes[FavoriteType::Item] ) ) == 0 ) {
        return FavoriteType::Item;
    }
    else if( strncmp( favoriteTypeName,
                      favoriteTypes[FavoriteType::Rule],
                      strlen( favoriteTypes[FavoriteType::Rule] ) ) == 0 ) {
        return FavoriteType::Rule;
    }
    else {
        FUNC_LOGI( "Debug: getType: wrong type of favorites" );
        return FavoriteType::TypeEnd;
    }
}

}  // namespace FavoriteManager

}  // namespace core
