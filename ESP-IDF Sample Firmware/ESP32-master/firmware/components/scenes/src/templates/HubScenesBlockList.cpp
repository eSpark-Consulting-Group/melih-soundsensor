#include "templates/HubScenesBlockList.h"
#include "Common.h"

#include "JsonStreamGenerator.h"
#include "gason.hpp"
#include "plugin_manager.h"
#include "FileStream.h"

#include "lua/responce/AddDevice.h"
#include "lua/responce/AddItem.h"
#include "lua/responce/HubSendBroadcast.h"
#include "lua/responce/RemoveDevice.h"

#include "Storage.h"

#include <string>
#include <cstring>
#include <tuple>

#include "ItemsFilterFinder.h"
#include "TmplGenerator.h"

//#define ENABLE_SCN_BLK_LOGS
#ifdef ENABLE_SCN_BLK_LOGS

#include "DebugUtils.h"

#define SCN_BLK_LOG_TAG "[SCN_BLK]"
#define SCN_BLK_LOGI( _FMT, ... ) ESP_LOGI( SCN_BLK_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define SCN_BLK_LOGE( _FMT, ... ) ESP_LOGE( SCN_BLK_LOG_TAG, _FMT, ##__VA_ARGS__ )
#define SCN_BLK_LOGW( _FMT, ... ) ESP_LOGW( SCN_BLK_LOG_TAG, _FMT, ##__VA_ARGS__ )

#else  // not ENABLE_SCN_BLK_LOGS

#define SCN_BLK_LOGI( ... ) \
    do {                    \
    } while( false )
#define SCN_BLK_LOGE( ... ) \
    do {                    \
    } while( false )
#define SCN_BLK_LOGW( ... ) \
    do {                    \
    } while( false )

#endif  // ENABLE_SCN_BLK_LOGS

namespace scenes {

using BlobType = storage::KeyValueStorage::BlobType;

extern const char SCENES_CACHE_PATH[];
extern const char SCENES_PATH[];

namespace {

const char TRIG_DEVICES_FNAME[] = "/blk_dev";
const char SCENES_BLOCKS_DATA_FNAME[] = "/blk_dat";
const char CACHED_DEVICE_JSON_BLOCK_PREFIX[] = "/devjs_";

common::IReadStream::UPtr createStream() {
    auto& pluginManager = plugin::PluginManager::get();
    auto pConf = pluginManager.getPluginConfig( tokens::SceneBlocks );
    auto blockPath = pConf.json( tokens::SceneBlocks2 ).toString();
    if( !blockPath ) {
        return nullptr;
    }
    auto pUri = pluginManager.getPluginURI( pConf.getPlugin() );
    return storage::ReadArchiveFileStream::open( pUri.uri.c_str(), blockPath );
}

bool checkFilter( const gateways::ItemInfo& itemInfo, const TemplateBlockData& blockData ) {
    const auto& filters = blockData.tokens();
    if( filters.end() == filters.find( itemInfo.getName().c_str() ) ) {
        return false;
    }

    gateways::BaseDeviceInfo::UPtr deviceInfo;

    const auto& prohibFilter = blockData.prohibitedDevices();
    if( !prohibFilter.empty() ) {
        deviceInfo = gateways::BaseDeviceInfo::create( itemInfo.getDeviceId() );
        if( !deviceInfo ) {
            return false;
        }
        auto itr = prohibFilter.find( deviceInfo->getCategory() );
        if( prohibFilter.end() != itr ) {
            const auto& subcats = itr->second;
            if( subcats.end() != subcats.find( deviceInfo->getSubcategory() ) ) {
                return false;
            }
        }
    }

    const auto& allowFilter = blockData.allowedDevices();
    if( !allowFilter.empty() ) {
        if( !deviceInfo ) {
            deviceInfo = gateways::BaseDeviceInfo::create( itemInfo.getDeviceId() );
            if( !deviceInfo ) {
                return false;
            }
        }

        auto itr = allowFilter.find( deviceInfo->getCategory() );
        if( allowFilter.end() != itr ) {
            const auto& subcats = itr->second;
            if( subcats.end() == subcats.find( deviceInfo->getSubcategory() ) ) {
                return false;
            }
        }
        else {
            return false;
        }
    }

    return true;
}

TokensSet getCachedHubScenesTriggerDevices() {
    auto fName = StringType( SCENES_CACHE_PATH ) + TRIG_DEVICES_FNAME;
    common::IReadStream::UPtr fStream = storage::ReadFileStream::open( fName.c_str() );
    TokensSet devices;
    fStream && fStream->read( devices );
    return std::move( devices );
}

bool saveCachedHubScenesTriggerDevices( const TokensSet& devices ) {
    auto fName = StringType( SCENES_CACHE_PATH ) + TRIG_DEVICES_FNAME;
    bool isWriteToTemporaryFileAndRenameOfOk = true;
    common::IWriteStream::UPtr fStream =
        storage::WriteFileStream::open( fName.c_str(), "wb+", isWriteToTemporaryFileAndRenameOfOk );
    return fStream && fStream->write( devices );
}

void enumerateScenesBlockData( std::function<bool( const TemplateBlockData& )> clbk ) {
    auto fName = StringType( SCENES_CACHE_PATH ) + SCENES_BLOCKS_DATA_FNAME;
    common::IReadStream::UPtr fStream = storage::ReadFileStream::open( fName.c_str() );
    if( !fStream ) {
        SCN_BLK_LOGI( "enumerateScenesBlockData: Can't open stream for scenes data blocks" );
        return;
    }

    TemplateBlockData bData;
    while( fStream->read( bData ) && clbk( bData ) ) {
        bData.reset();
    }
}

bool removeCachedDeviceJsFile( const StringType& deviceId ) {
    auto fName = StringType( SCENES_CACHE_PATH ) + CACHED_DEVICE_JSON_BLOCK_PREFIX + deviceId.c_str();
    return storage::Storage::get().remove( fName.c_str() );
}

bool updateScenesBlockData( const bool isRebuild = false ) {
    auto fName = StringType( SCENES_CACHE_PATH ) + SCENES_BLOCKS_DATA_FNAME;
    if( !isRebuild && storage::Storage::get().isExist( fName ) ) {
        return true;
    }

    bool isWriteToTemporaryFileAndRenameOfOk = true;
    common::IWriteStream::UPtr wStreamSptr =
        storage::WriteFileStream::open( fName.c_str(), "wb+", isWriteToTemporaryFileAndRenameOfOk );
    if( !wStreamSptr ) {
        return false;
    }
    auto& wStream = *wStreamSptr;

    auto blocksStream = createStream();
    if( !blocksStream ) {
        return false;
    }

    auto genClbk = [&wStream]( const TemplateBlockData& bData ) -> ItemsFilterFinder::Mode {
        return wStream.write( bData ) ? ItemsFilterFinder::Mode::InWork : ItemsFilterFinder::Mode::Failed;
    };

    ItemsFilterFinder finder( std::move( blocksStream ), genClbk );
    if( !finder.find( tokens::When ) ) {
        return false;
    }

    if( !finder.reset() ) {
        return false;
    }

    if( !finder.find( tokens::Then ) ) {
        return false;
    }

    return true;
}

std::tuple<bool, bool> generateTemplateFileFor( const gateways::BaseDeviceInfo& deviceInfo,
                                                const std::list<gateways::ItemInfo>& itemsInfo ) {
    auto outputfName =
        StringType( SCENES_CACHE_PATH ) + CACHED_DEVICE_JSON_BLOCK_PREFIX + StringType( deviceInfo.getId() );
    bool isWriteToTemporaryFileAndRenameOfOk = true;
    auto m_oStream = storage::WriteFileStream::open( outputfName.c_str(), "wb+", isWriteToTemporaryFileAndRenameOfOk );
    if( !m_oStream ) {
        return std::make_tuple( false, false );
    }

    auto m_gen = std::make_unique<common::JsonStreamGenerator>( *m_oStream );
    if( !m_gen ) {
        return std::make_tuple( false, false );
    }
    // imitate that JsonStreamGenerator inside the json array
    m_gen->setFirstItemState();
    m_gen->setDept( 1 );

    auto m_tmplGen = std::make_unique<TemplatesGenerator>( *m_gen, createStream(), deviceInfo, itemsInfo );
    if( !m_tmplGen ) {
        return std::make_tuple( false, false );
    }

    bool m_isTriggerDevice = false;
    for( const auto& itemInfo : itemsInfo ) {
        bool m_isSuccess = true;
        auto clbk =
            [&m_isTriggerDevice, &m_isSuccess, &itemInfo, &m_tmplGen]( const TemplateBlockData& bData ) -> bool {
            if( !checkFilter( itemInfo, bData ) ) {
                return true;
            }
            m_isTriggerDevice = true;
            m_isSuccess = m_tmplGen->generate( itemInfo, bData );
            return m_isSuccess;
        };
        enumerateScenesBlockData( clbk );

        if( !m_isSuccess ) {
            return std::make_tuple( false, m_isTriggerDevice );
        }
    }

    // imitate that JsonStreamGenerator inside the json array
    m_gen->setDept( 0 );

    return std::make_tuple( true, m_isTriggerDevice );
}

bool removeFromHubScenesTriggerDevicesList( const std::string& deviceId ) {
    auto devices = getCachedHubScenesTriggerDevices();
    devices.erase( deviceId.c_str() );
    bool isSuccess = saveCachedHubScenesTriggerDevices( devices );
    if( removeCachedDeviceJsFile( deviceId.c_str() ) ) {
        isSuccess = false;
    }

    return isSuccess;
}

}  // namespace

bool updateHubScenesTriggerDevicesList( const bool isRebuild ) {
    if( isRebuild ) {
        storage::Storage::get().remove( SCENES_CACHE_PATH );
        storage::Storage::get().createDir( SCENES_CACHE_PATH );
    }

    if( !updateScenesBlockData( isRebuild ) ) {
        return false;
    }

    TokensSet devices;
    {
        auto clbk = [&devices]( const TemplateBlockData& bData ) -> bool {
            gateways::enumerateItemsFromDb( [&devices, &bData]( const gateways::ItemInfo& info ) -> bool {
                if( checkFilter( info, bData ) ) {
                    devices.emplace( info.getDeviceId() );
                }
                return true;
            } );
            return true;
        };
        enumerateScenesBlockData( clbk );
    }

    {
        auto cachedDevices = getCachedHubScenesTriggerDevices();
        for( const auto& cachedDevId : cachedDevices ) {
            auto fName = StringType( SCENES_CACHE_PATH ) + CACHED_DEVICE_JSON_BLOCK_PREFIX + cachedDevId;
            if( storage::Storage::get().isExist( fName ) ) {
                devices.erase( cachedDevId );
            }
        }
    }

    auto trigDevices = getCachedHubScenesTriggerDevices();

    for( const auto& deviceId : devices ) {
        auto deviceInfo = gateways::BaseDeviceInfo::create( deviceId );
        if( !deviceInfo ) {
            continue;
        }

        std::list<gateways::ItemInfo> deviceItems;
        gateways::enumerateItemsFromDb( [&deviceItems, &deviceInfo]( const gateways::ItemInfo& info ) -> bool {
            if( info.getDeviceId() == deviceInfo->getId() ) {
                deviceItems.emplace_back( info );
            }
            return true;
        } );

        auto [isSuccess, isTriggered] = generateTemplateFileFor( *deviceInfo, deviceItems );
        if( !isSuccess ) {
            removeCachedDeviceJsFile( deviceId );
            return false;
        }

        if( isTriggered ) {
            trigDevices.emplace( deviceId.c_str() );
        }
        else {
            removeCachedDeviceJsFile( deviceId );
            return false;
        }
    }

    return saveCachedHubScenesTriggerDevices( trigDevices );
}

std::tuple<int, std::string> onHubScenesBlocksList( const gason::JsonValue& json, common::JsonStreamGenerator& gen ) {
    auto blockType = json( tokens::Params )( tokens::BlockType ).toString();
    if( !blockType ) {
        gen.writeEmptyObject() && gen.writeItemDelimiter();
        return std::make_tuple( errors::InvalidParams, "Invalid \"blockType\"" );
    }

    std::set<std::string> devices;
    {
        auto devicesJsList = json( tokens::Params )( tokens::Devices );
        auto end = gason::end( devicesJsList );
        for( auto itr = gason::begin( devicesJsList ); itr != end; ++itr ) {
            auto devId = itr->value.toString();
            if( !devId ) {
                continue;
            }
            devices.emplace( devId );
        }
    }

    if( !gen.writeOpeningObject() || !gen.writePropertyKey( blockType ) || !gen.writeOpeningArray() ) {
        gen.writeEmptyObject() && gen.writeItemDelimiter();
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    if( devices.empty() ) {
        gen.writeClosingSymbol() && gen.writeClosingSymbol();
        return std::make_tuple( errors::InvalidParams, "Invalid \"devices\"" );
    }

    bool isSuccess = true;
    for( const auto& deviceId : devices ) {
        auto jsFName = StringType( SCENES_CACHE_PATH ) + CACHED_DEVICE_JSON_BLOCK_PREFIX + deviceId.c_str();
        common::IReadStream::UPtr fStream1 = storage::ReadFileStream::open( jsFName.c_str() );
        if( !fStream1 ) {
            continue;
        }

        isSuccess = gen.writeRawProperty( nullptr, *fStream1 );
        if( !isSuccess ) {
            break;
        }
    }

    if( !isSuccess ) {
        gen.writeClosingSymbol() && gen.writeClosingSymbol();
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    if( !gen.writeClosingSymbol() || !gen.writeClosingSymbol() ) {
        return std::make_tuple( errors::GenJsonFailed, StringType{} );
    }

    return std::make_tuple( errors::NotError, "" );
}

std::tuple<int, std::string> onHubScenesTriggerDevicesList( common::JsonStreamGenerator& gen ) {
    const auto devices = getCachedHubScenesTriggerDevices();
    if( gen.writeOpeningObject() && gen.writeProperty( tokens::Devices, devices ) && gen.writeClosingSymbol() ) {
        return std::make_tuple( errors::NotError, "" );
    }
    return std::make_tuple( errors::GenJsonFailed, StringType{} );
}

bool ScenesTemplates::initializeImpl() {
    SCN_BLK_LOGI( "ScenesTemplates::initializeImpl()" );

    if( !core::Shutdownable::initializeImpl() ) {
        SCN_BLK_LOGE( "ScenesTemplates::initializeImpl(): Can't initialize BusClient" );
        return false;
    }

    if( !bus::Bus::get().join( lua::responce::BUS_NAME, *this ) ) {
        SCN_BLK_LOGE( "ScenesTemplates::initialize(): Can't join to lua::responce::BUS_NAME" );
        return false;
    }

    SCN_BLK_LOGI( "ScenesTemplates::initialize() Instance successfully initialized" );
    return true;
}

void ScenesTemplates::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddDevice& command ) {
    SCN_BLK_LOGI( "ScenesTemplates::visit( command::AddDevice )" );

    if( command.m_deviceInfo ) {
        m_devIdToDevicesInfo.emplace( command.m_deviceInfo->getId(), *command.m_deviceInfo );
    }
}

void ScenesTemplates::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddItem& command ) {
    SCN_BLK_LOGI( "ScenesTemplates::visit( lua::responce::AddItem )" );

    if( command.m_itemInfo ) {
        auto& itemsInfoList = m_devIdToItemsInfo[command.m_itemInfo->getDeviceId()];
        itemsInfoList.push_back( *command.m_itemInfo );
    }
}

void ScenesTemplates::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::HubSendBroadcast& cmd ) {
    SCN_BLK_LOGI( "ScenesTemplates::visit( lua::responce::HubSendBroadcast m_plugin=%s, m_event=%s, m_key=%s)",
                  cmd.m_plugin.c_str(),
                  cmd.m_event.c_str(),
                  cmd.m_key.c_str() );
#ifdef ENABLE_SCN_BLK_LOGS
    for( const auto& value : cmd.m_value ) {
        SCN_BLK_LOGI( "ScenesTemplates::visit( lua::responce::HubSendBroadcast value=%s)", value.c_str() );
    }
#endif  // ENABLE_SCN_BLK_LOGS
    if( common::resources::broadcasts::events::IncludeStarted == cmd.m_event ) {
        clearState();
    }
    else if( common::resources::broadcasts::events::IncludeFinished == cmd.m_event ) {
        onIncludeFinished( std::move( m_devIdToDevicesInfo ), std::move( m_devIdToItemsInfo ) );
    }
    else if( common::resources::broadcasts::events::IncludeFinishedError == cmd.m_event ||
             common::resources::broadcasts::events::IncludeTimeout == cmd.m_event ) {
        onIncludeFailed( std::move( m_devIdToDevicesInfo ), std::move( m_devIdToItemsInfo ) );
    }
}

void ScenesTemplates::visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::RemoveDevice& command ) {
    SCN_BLK_LOGI( "ScenesTemplates::visit( lua::responce::RemoveDevice )" );

    removeFromHubScenesTriggerDevicesList( command.m_deviceId );
}

void ScenesTemplates::onIncludeFinished( DevIdToDevInfoMap&& devicesInfoMap, DevIdToItemsMap&& itemsInfoMap ) {
    if( devicesInfoMap.empty() || itemsInfoMap.empty() ) {
        SCN_BLK_LOGE( "ScenesTemplates::onIncludeFinished bad data" );
        return;
    }
    SCN_BLK_LOGI( "ScenesTemplates::onIncludeFinished start" );

    if( !updateScenesBlockData() ) {
        SCN_BLK_LOGE( "ScenesTemplates::onIncludeFinished: Can't update scene blocks" );
        return;
    }

    for( const auto& devKeyValPair : devicesInfoMap ) {
        const auto& deviceId = devKeyValPair.first;
        const auto& deviceInfo = devKeyValPair.second;
        SCN_BLK_LOGI( "\t\tdeviceName=%s, deviceId=%s ", deviceInfo.getName(), deviceInfo.getId() );

        const auto& itemsInfo = itemsInfoMap[deviceId];
        if( itemsInfo.empty() ) {
            SCN_BLK_LOGW( "\t\tItems list is empty, skip it" );
            continue;
        }
        else {
            SCN_BLK_LOGI( "\t\titemsCount=%d", itemsInfo.size() );
        }

        auto [isSuccess, isTriggerDevice] = generateTemplateFileFor( deviceInfo, itemsInfo );
        if( !isSuccess ) {
            SCN_BLK_LOGE( "ScenesTemplates::onIncludeFinished: Can't generate template file for device items" );
            return;
        }

        if( !isTriggerDevice ) {
            SCN_BLK_LOGW( "ScenesTemplates::onIncludeFinished: Blocks did not founded" );
            removeCachedDeviceJsFile( deviceInfo.getId() );
            return;
        }

        auto devices = getCachedHubScenesTriggerDevices();
        devices.emplace( deviceInfo.getId() );
        if( !saveCachedHubScenesTriggerDevices( devices ) ) {
            SCN_BLK_LOGE( "ScenesTemplates::onIncludeFinished: Can't generate template file for device items" );
            removeCachedDeviceJsFile( deviceInfo.getId() );
        }
    }

    SCN_BLK_LOGI( "ScenesTemplates::onIncludeFinished: Successfully completed" );
}

void ScenesTemplates::onIncludeFailed( DevIdToDevInfoMap&& devicesInfo, DevIdToItemsMap&& itemsInfo ) {
    SCN_BLK_LOGI( "ScenesTemplates::onIncludeFailed" );
#ifdef ENABLE_SCN_BLK_LOGS
    for( const auto& keyValPair : devicesInfo ) {
        const auto& deviceInfo = keyValPair.second;
        SCN_BLK_LOGI( "\t\tdeviceName=%s, deviceId=%s, itemsCount=%d\n",
                      deviceInfo.getName(),
                      deviceInfo.getId(),
                      itemsInfo.size() );
    }
#endif  // ENABLE_SCN_BLK_LOGS
}

void ScenesTemplates::clearState() {
    m_devIdToDevicesInfo.clear();
    m_devIdToItemsInfo.clear();
}

}  // namespace scenes
