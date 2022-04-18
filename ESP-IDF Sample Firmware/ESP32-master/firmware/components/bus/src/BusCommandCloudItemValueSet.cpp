#include <Bus.h>
#include <BusCommandCloudItemValueSet.h>
#include <memory>
#include <algorithm>
#include <GatewayUtils.h>
#include <DebugUtils.h>
#include <DeviceInfo.h>

namespace bus {
namespace command {

bool CloudItemValueSet::send( const char* receiver,
                              const IBusClient& client,
                              ListOfItemId&& items,
                              common::IValue::UPtr&& value ) {
    if( !value ) {
        FUNC_LOGE( "Value is null!" );
        return false;
    }

    if( items.empty() ) {
        return false;
    }

    std::map<std::string, ListOfItemId> pluginNameToItemId;
    for( auto itr = items.cbegin(); itr != items.cend(); ++itr ) {
        auto pluginName = gateways::utils::getPluginNameByItemId( itr->c_str() );
        if( pluginName.empty() ) {
            FUNC_LOGW( "Can't get a plugin's name for item's id=%s, skip it", itr->c_str() );
            continue;
        }

        auto& itemsIds = pluginNameToItemId[pluginName];
        itemsIds.emplace_back( *itr );
    }

    if( pluginNameToItemId.empty() ) {
        return false;
    }

    for( auto itr = pluginNameToItemId.begin(); itr != pluginNameToItemId.end(); ++itr ) {
        auto itemsIds = itr->second;
        while( !itemsIds.empty() ) {
            auto itemInfo = gateways::ItemInfo::create( *itemsIds.begin() );
            std::string item_name = itemInfo->getName();

            ListOfItemId itemsList;
            for( auto item = itemsIds.begin(); item != itemsIds.end(); ) {
                auto itemInfo = gateways::ItemInfo::create( *item );
                if( item_name == itemInfo->getName() ) {
                    // group item by itemName
                    itemsList.emplace_back( *item );
                    item = itemsIds.erase( item );
                }
                else {
                    item++;
                }
            }

            client.send<CloudItemValueSet>( receiver, itr->first, itemsList, value->clone() );
        }
    }

    return true;
}

}  // namespace command
}  // namespace bus
