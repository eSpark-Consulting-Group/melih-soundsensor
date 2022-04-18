#pragma once

#include <DeviceInfo.h>
#include <chrono>

namespace nma {

struct UpdateItemInfo {
    std::string id = "";
    std::string category = "General";
    std::chrono::time_point<std::chrono::steady_clock> lastUpdateTime = std::chrono::steady_clock::now();
    bool status = false;
};

class ItemsUpdatdBroadcastManager {
public:
    ~ItemsUpdatdBroadcastManager();
    static ItemsUpdatdBroadcastManager& get();
    bool canSendItemUpdate( const gateways::ItemInfo& itemInfo );

private:
    ItemsUpdatdBroadcastManager() {}

    int delayForItem( UpdateItemInfo item );
    UpdateItemInfo* addItemIfNeeded( const gateways::ItemInfo& itemInfo );

private:
    std::vector<UpdateItemInfo*> m_items;
};

}  // namespace nma
