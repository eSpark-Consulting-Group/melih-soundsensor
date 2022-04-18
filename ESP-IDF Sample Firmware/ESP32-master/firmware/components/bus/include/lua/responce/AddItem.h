#pragma once

#include <DeviceInfo.h>
#include <BusCommand.h>
#include <memory>

namespace lua {
namespace responce {

struct AddItem final : public bus::IBusCommand {
    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::unique_ptr<gateways::ItemInfo> m_itemInfo;
};

}  // namespace responce
}  // namespace lua
