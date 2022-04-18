
#include <PagesManager.h>

#include <JsonStreamGenerator.h>
#include <CommonResources.h>

#include <BinaryFileStorageFactory.h>
#include <StorageSection.h>

#include <Utils.h>
#include <HardUtils.h>

#include <BusCommandRoomsCreated.h>
#include <BusCommandRoomsDeleted.h>
#include <BusCommandRoomsReordered.h>

#include <cstring>
#include <algorithm>
#include <mutex>

//#define ENABLE_PMR_LOGS
#ifdef ENABLE_PMR_LOGS

#include <DebugUtils.h>

#define PMR_LOG_TAG "[PagesManager]"
#define PMR_LOGI( _FMT, ... ) ESP_LOGI( PMR_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define PMR_LOGE( _FMT, ... ) ESP_LOGE( PMR_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define PMR_LOGW( _FMT, ... ) ESP_LOGW( PMR_LOG_TAG, _FMT, ##__VA_ARGS__ )

#else

#define PMR_LOGI( ... ) \
    do {                \
    } while( false )
#define PMR_LOGE( ... ) \
    do {                \
    } while( false )
#define PMR_LOGW( ... ) \
    do {                \
    } while( false )

#endif

namespace core {

using namespace common::resources;
using BlobType = storage::KeyValueStorage::BlobType;

constexpr char PagesManager::BedroomName[];
constexpr char PagesManager::LivingroomName[];

namespace {

std::string pageIdToString( const PagesManager::PageId pageId ) {
    char buf[16];
    snprintf( buf, sizeof( buf ), "%" PRIX32, pageId );
    return buf;
}

}  // end of unnamed namespace

PagesManager::~PagesManager() {
    PMR_LOGI( "call destructor" );
}

bool PagesManager::initializeImpl() {
    if( !ParentType::initializeImpl() ) {
        PMR_LOGE( "Can't initialize ParentType" );
        return false;
    }

    if( !bus::Bus::get().join( "pages_man", *this ) ) {
        PMR_LOGE( "scene bus's client has not been joined" );
        return false;
    }

    PMR_LOGI( "Initialized successfully" );
    return true;
}

bool PagesManager::isPageWithId( const PageId& pageId ) {
    PMR_LOGI( "isPageWithId id=0x%04X", pageId );
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    return stor && stor->exists( pageIdToString( pageId ) );
}

bool PagesManager::isPageWithName( const std::string& name ) {
    PMR_LOGI( "isPageWithName name=%s", name.c_str() );
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    if( !stor ) {
        return false;
    }
    auto itr = stor->cbegin();
    auto end = stor->cend();
    while( itr != end ) {
        auto value = itr.value();
        if( !value.is_type<BlobType>() ) {
            return false;
        }
        auto blob = value.get<BlobType>();
        if( blob.empty() ) {
            return false;
        }

        if( blob.data() == name ) {
            return true;
        }

        ++itr;
    }

    return false;
}

bool PagesManager::setPageName( const PageId& pageId, const std::string& name, bool is_edited ) {
    PMR_LOGI( "setPageName id=0x%04X, name=%s", pageId, name.c_str() );
    if( 0 == name.length() ) {
        return false;
    }

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    if( !stor )
        return false;

    const std::string key = pageIdToString( pageId );
    BlobType blob( name.length() + 1 );
    std::strncpy( blob.data(), name.c_str(), blob.size() );

    bool result = stor->setBlob( key, blob );

    if( result ) {
        auto cmd = std::make_unique<bus::command::RoomsCreated>();
        if( cmd ) {
            cmd->m_edited = is_edited;
            cmd->m_roomId = std::move( key );
            cmd->m_name = name;
            send( "nma", std::move( cmd ) );
        }
    }

    return result;
}

std::tuple<int, std::string> PagesManager::createPage( const std::string& name, PageId& resultId ) {
    PMR_LOGI( "createPage name=%s", name.c_str() );

    if( 0 == name.length() ) {
        return std::make_tuple( errors::InvalidRequest, errors::data::RpcParamsEmptyName );
    }

    auto truncName = name.substr( 0, MaxNameLen );

    if( isPageWithName( truncName ) ) {
        return std::make_tuple( errors::ApplicationError, errors::data::HubRoomAlreadyExist );
    }

    resultId = hard::common::generateUniqueId();
    if( setPageName( resultId, truncName, false ) ) {
        return std::make_tuple( errors::NotError, "" );
    }
    else {
        return std::make_tuple( errors::ApplicationError, "" );
    }
}

bool PagesManager::deletePage( const PageId& pageId ) {
    PMR_LOGI( "deletePage id=0x%04X", pageId );
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    if( !stor )
        return false;

    const std::string key = pageIdToString( pageId );
    bool result = stor->remove( key );

    if( result ) {
        auto cmd = std::make_unique<bus::command::RoomsDeleted>();
        if( cmd ) {
            cmd->m_deletedRoom = std::move( key );
            send( "nma", std::move( cmd ) );
        }
    }

    return result;
}

bool PagesManager::deleteAllPages() {
    PMR_LOGI( "deleteAllPages" );
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    return stor && stor->removeAll();
}

bool PagesManager::listPages( common::JsonStreamGenerator& jsonGenerator ) {
    PMR_LOGI( "List all pages to json" );
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    if( !stor )
        return false;

    auto itr = stor->cbegin();
    auto end = stor->cend();
    for( ; itr != end; ++itr ) {
        if( !jsonGenerator.writeOpeningObject() ) {
            return false;
        }

        auto key_str = *itr;
        if( !jsonGenerator.writeProperty( tokens::SubId, key_str ) ) {
            return false;
        }

        auto value = itr.value();
        if( !value.is_type<BlobType>() ) {
            return false;
        }
        auto blob = value.get<BlobType>();
        if( blob.empty() ) {
            return false;
        }

        if( !jsonGenerator.writeProperty( tokens::Name, blob.data() ) ) {
            return false;
        }

        if( !jsonGenerator.writeClosingSymbol() ) {
            return false;
        }

        PMR_LOGI( "list room with id=%s, name=%s", key_str.c_str(), blob.data() );
    }

    return true;
}

bool PagesManager::listPagesWithFilter( common::JsonStreamGenerator& jsonGenerator, std::vector<std::string> ids ) {
    PMR_LOGE( "List all pages to json ids : %d", ids.size() );
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    if( !stor )
        return false;

    auto itr = stor->cbegin();
    auto end = stor->cend();
    for( ; itr != end; ++itr ) {
        auto key_str = *itr;

        if( ids.end() != std::find( ids.begin(), ids.end(), key_str ) ) {
            if( !jsonGenerator.writeOpeningObject() ) {
                return false;
            }

            if( !jsonGenerator.writeProperty( tokens::SubId, key_str ) ) {
                return false;
            }

            auto value = itr.value();
            if( !value.is_type<BlobType>() ) {
                return false;
            }
            auto blob = value.get<BlobType>();
            if( blob.empty() ) {
                return false;
            }

            if( !jsonGenerator.writeProperty( tokens::Name, blob.data() ) ) {
                return false;
            }

            if( !jsonGenerator.writeClosingSymbol() ) {
                return false;
            }
        }

        PMR_LOGI( "list room with id=%s", key_str.c_str() );
    }

    return true;
}

bool PagesManager::listPage( common::JsonStreamGenerator& jsonGenerator, const PageId& pageId ) {
    PMR_LOGI( "List page with id=0x%04X to json", pageId );
    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    if( !stor )
        return false;

    auto result = jsonGenerator.writeOpeningObject();

    const std::string key = pageIdToString( pageId );
    result = result && jsonGenerator.writeProperty( common::resources::tokens::SubId, key );

    storage::KeyValueStorage::BlobType blob;
    if( result && stor->getBlob( key, blob ) ) {
        result = result && jsonGenerator.writeProperty( common::resources::tokens::Name, blob.data() );
    }

    return result && jsonGenerator.writeClosingSymbol();
}

bool PagesManager::setPageOrder( const PageId* pageIds, uint8_t count ) {
    PMR_LOGI( "setPageOrder not implemented yet!" );
    //#TODO Not implemented yet
    return false;
}

std::string PagesManager::getPageName( const PageId& pageId ) {
    PMR_LOGI( "getPageName id=0x%04X", pageId );

    auto stor = storage::BinaryFileStorageFactory::get().openNamespace( storage::section::ROOMS );
    if( !stor )
        return {};

    const std::string key = pageIdToString( pageId );
    storage::KeyValueStorage::BlobType blob;
    if( !stor->getBlob( key, blob ) ) {
        return {};
    }

    return blob.data();
}

std::string PagesManager::getPageName( const std::string& pageId ) {
    PMR_LOGI( "getPageName(char) id=%s", pageId.c_str() );
    PageId id = 0;
    if( !pageId.empty() && 0 < sscanf( pageId.c_str(), "%" PRIX32, &id ) ) {
        return getPageName( id );
    }

    return {};
}

}  // namespace core
