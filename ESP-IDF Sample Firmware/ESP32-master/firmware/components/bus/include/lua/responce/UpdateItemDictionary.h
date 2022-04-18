#pragma once

#include <string>
#include <memory>
#include <IValue.h>
#include <BusCommand.h>
#include <DeviceInfo.h>

namespace lua {
namespace responce {

struct UpdateItemDictionary final : public bus::IBusCommand {
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }

    std::unique_ptr<gateways::ItemInfo> m_itemInfo;
    std::string m_operation;
    std::string m_itemDictionaryId;
    common::IValue::UPtr m_value;
};

}  // namespace responce
}  // namespace lua
