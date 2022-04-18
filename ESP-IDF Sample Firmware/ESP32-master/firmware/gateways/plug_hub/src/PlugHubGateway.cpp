#include <PlugHubGateway.h>
#include <PlugHubManager.h>
#include <PlugHub.h>

namespace gateways {
namespace plug_hub {

Gateway& Gateway::get() {
    static Gateway plug_hub;
    return plug_hub;
}

Gateway::~Gateway() = default;

GatewayID Gateway::getTypeId() const {
    return GatewayID::PlugHub;
}

Gateway::ClbkRet Gateway::updateComponent( UpdContext& ctx ) {
    gateways::plug_hub::PlugHubManager::get().onLoop();

    return ClbkRet::Continue;
}

size_t Gateway::getRecommendedThreadStackSize() const {
    return 4096;
}

bool Gateway::initialize() {
    return PlugHubManager::get().initialize() && PlugHub::get().initialize();
}

void Gateway::startup( ScriptExecutor scriptExecutor ) {
    PlugHub::get().startup( scriptExecutor );
}

Gateway::AddingDeviceStatus Gateway::restoreDeviceInfo( const DeviceId&, ::storage::File& ) {
    return AddingDeviceStatus::OK;
}

}  // namespace plug_hub
}  // namespace gateways
