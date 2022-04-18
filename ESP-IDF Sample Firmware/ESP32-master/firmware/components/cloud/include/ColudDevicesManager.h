#ifdef CONFIG_CLOUD_DEVICES

#include <algorithm>
#include <Bus.h>
#include <BusCommandSetHandler.h>
#include <IGateway.h>
#include <CloudDevice.h>
#include <CommonResources.h>
#include "BusEventsHandler.h"

#include <map>

namespace gateways {
namespace cloud {

class ColudDevicesManager : public bus::IBusClient, public common::IInitialize {
    static constexpr auto BUS_CLIENT_NAME = "cloud_devices";
    static constexpr int BUS_WAIT_TIME_MS = 0;

public:
    static ColudDevicesManager& get();

    void startup( gateways::IGateway::ScriptExecutor scriptExecutor );

    bool storeDevice( std::unique_ptr<Device>& device, const char version );
    bool removeDevice( std::string& id );
    Device getDeviceInfo( const std::string& id, IGateway::AddingDeviceStatus& addingStatus );

    void deviceAdded( std::string& id );
    void deviceRemoved( std::string& id );
    void deviceResponseCommand( std::string& commandId, std::string& result, std::string& error );
    void deviceUpdate( std::string& id, std::map<std::string, std::string>& fields );

    bool onLoop();

private:
    ColudDevicesManager();

    bool initializeImpl() override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) override;

private:
    bool m_allDevicesRemoved;
    bus::EventsHandler m_eventsHandler;
};

}  // namespace cloud
}  // namespace gateways

#endif  // CONFIG_CLOUD_DEVICES
