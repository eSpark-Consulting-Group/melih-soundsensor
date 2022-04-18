#include "ItemsUpdatdBroadcastManager.h"
#include <CommonResources.h>
#include "HubSettings.h"

namespace nma {

ItemsUpdatdBroadcastManager::~ItemsUpdatdBroadcastManager() {
    std::for_each( m_items.begin(), m_items.end(), []( auto item ) { delete item; } );
}

ItemsUpdatdBroadcastManager& ItemsUpdatdBroadcastManager::get() {
    static ItemsUpdatdBroadcastManager manager;
    return manager;
}

UpdateItemInfo* ItemsUpdatdBroadcastManager::addItemIfNeeded( const gateways::ItemInfo& itemInfo ) {
    for( auto item : m_items ) {
        if( item->id == itemInfo.getId() ) {
            item->status = false;
            return item;
        }
    }

    auto updateInfo = new UpdateItemInfo();
    updateInfo->id = itemInfo.getId();
    updateInfo->category = itemInfo.getCategory();
    updateInfo->status = true;
    m_items.push_back( updateInfo );

    return updateInfo;
}

bool ItemsUpdatdBroadcastManager::canSendItemUpdate( const gateways::ItemInfo& itemInfo ) {
    auto updateItem = addItemIfNeeded( itemInfo );
    const auto timePoint = std::chrono::steady_clock::now();

    if( updateItem->status ) {
        updateItem->lastUpdateTime = timePoint;
        return true;
    }

    std::map<std::string, int> dels = core::HubSettings::get().getBroadcastDelays();
    auto delayForCategory = dels[updateItem->category];
    auto delay = std::chrono::seconds( delayForCategory );

    if( timePoint >= updateItem->lastUpdateTime + delay ) {
        updateItem->lastUpdateTime = timePoint;
        return true;
    }

    return false;
}

}  // namespace nma
