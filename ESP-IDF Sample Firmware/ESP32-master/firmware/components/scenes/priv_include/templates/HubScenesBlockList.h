#pragma once

#include "Shutdownable.h"
#include "DeviceInfo.h"

#include <tuple>
#include <string>
#include <list>
#include <map>

namespace gason {
class JsonValue;
}  // namespace gason

namespace common {
class JsonStreamGenerator;
}  // namespace common

namespace scenes {

class ScenesTemplates : public core::Shutdownable {
public:
    using ClbkRet = core::Shutdownable::ClbkRet;
    using ItemsInfoList = std::list<gateways::ItemInfo>;
    using DevInfoUPtr = gateways::BaseDeviceInfo::UPtr;

    using DevIdToDevInfoMap = std::map<std::string, gateways::BaseDeviceInfo>;
    using DevIdToItemsMap = std::map<std::string, ItemsInfoList>;

protected:  // bus::IBusCommandVisitor
    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override {
        return ClbkRet::Continue;
    }

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddDevice& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::AddItem& command ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::HubSendBroadcast& cmd ) override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, lua::responce::RemoveDevice& command ) override;

private:
    void onIncludeFinished( DevIdToDevInfoMap&& devicesInfo, DevIdToItemsMap&& itemsInfo );

    void onIncludeFailed( DevIdToDevInfoMap&& devicesInfo, DevIdToItemsMap&& itemsInfo );

    void clearState();

private:
    DevIdToDevInfoMap m_devIdToDevicesInfo;
    DevIdToItemsMap m_devIdToItemsInfo;
};

}  // namespace scenes
