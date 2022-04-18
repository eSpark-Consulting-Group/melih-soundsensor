#ifdef CONFIG_CLOUD_DEVICES

#include <ColudDevicesManager.h>
#include <Storage.h>
#include <StorageFile.h>
#include <NotificationEvent.h>
#include "plugin_manager.h"
#include "DebugUtils.h"

#include <BusCommandSetHandler.h>

#include <cloud/responses/BusCommandHandleEventCloudDeviceAdded.h>
#include <cloud/responses/BusCommandHandleEventCloudDeviceRemoved.h>
#include <cloud/responses/BusCommandHandleEventCloudDeviceUpdated.h>
#include <cloud/responses/BusCommandHandleEventCloudResponseCommand.h>

#include "lua/responce/HubSendBroadcast.h"
#include <BusCommandStartExclude.h>
#include <BusCommandPluginRun.h>

#include <CloudDevicesStorage.h>

namespace gateways {

namespace cloud {

ColudDevicesManager::ColudDevicesManager()
: m_eventsHandler( { 0, "" } ) {
    if( !initialize() ) {
        FUNC_LOGE( "cloud bus's client has not been initialized\n" );
    }
}

ColudDevicesManager& ColudDevicesManager::get() {
    static ColudDevicesManager cl;
    return cl;
}

bool ColudDevicesManager::storeDevice( std::unique_ptr<gateways::cloud::Device>& device, const char version ) {
    if( DevicesStorage::get().addDeviceFile( device, version ) ) {
        return true;
    }

    return false;
}

bool ColudDevicesManager::removeDevice( std::string& id ) {
    if( DevicesStorage::get().removeDeviceFile( id ) ) {
        return true;
    }

    return false;
}

gateways::cloud::Device ColudDevicesManager::getDeviceInfo( const std::string& id,
                                                            IGateway::AddingDeviceStatus& addingStatus ) {
    return DevicesStorage::get().restoreDevice( id, addingStatus );
}

bool ColudDevicesManager::initializeImpl() {
    return bus::Bus::get().join( BUS_CLIENT_NAME, *this );
}

bool ColudDevicesManager::onLoop() {
    tryReceive( BUS_WAIT_TIME_MS / portTICK_PERIOD_MS );
    return true;
}

void ColudDevicesManager::startup( gateways::IGateway::ScriptExecutor scriptExecutor ) {
    auto& pluginManager = plugin::PluginManager::get();
    auto pConf = pluginManager.getPluginConfig( "cloud_devices" );
    const char* name = pConf.json( "startup" ).toString();
    if( name ) {
        auto script_name = std::string( "cloud_devices/scripts/" ) + "startup" + std::string( ".lua" );
        scriptExecutor( script_name );
    }
}

void ColudDevicesManager::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) {
    if( command.isSubscribe() ) {
        m_eventsHandler = { from, command.handlerName() };
    }
    else {
        m_eventsHandler = { bus::BusAddrInvalid, "" };
    }
}

void ColudDevicesManager::deviceAdded( std::string& id ) {
    auto bus_command = std::make_unique<::cloud::responses::HandleEventCloudDeviceAdded>(
        std::move( id ), m_eventsHandler.m_handlerName, "device_added", 0 );
    send( m_eventsHandler.m_addr, std::move( bus_command ) );
}

void ColudDevicesManager::deviceRemoved( std::string& id ) {
    auto bus_command = std::make_unique<::cloud::responses::HandleEventCloudDeviceRemoved>(
        std::move( id ), m_eventsHandler.m_handlerName, "device_removed", 0 );
    send( m_eventsHandler.m_addr, std::move( bus_command ) );
}

void ColudDevicesManager::deviceResponseCommand( std::string& commandId, std::string& result, std::string& error ) {
    auto bus_command =
        std::make_unique<::cloud::responses::HandleEventCloudResponseCommand>( std::move( commandId ),
                                                                               std::move( result ),
                                                                               std::move( error ),
                                                                               m_eventsHandler.m_handlerName,
                                                                               "command_response",
                                                                               0 );
    send( m_eventsHandler.m_addr, std::move( bus_command ) );
}

void ColudDevicesManager::deviceUpdate( std::string& id, std::map<std::string, std::string>& fields ) {
    auto bus_command = std::make_unique<::cloud::responses::HandleEventCloudDeviceUpdated>(
        std::move( id ), std::move( fields ), m_eventsHandler.m_handlerName, "device_updated", 0 );
    send( m_eventsHandler.m_addr, std::move( bus_command ) );
}

}  // namespace cloud
}  // namespace gateways

#endif  // CONFIG_CLOUD_DEVICES
