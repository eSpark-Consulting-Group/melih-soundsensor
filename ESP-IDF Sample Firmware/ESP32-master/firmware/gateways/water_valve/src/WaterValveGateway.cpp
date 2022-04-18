#include <WaterValveGateway.h>
#include <WaterValveManager.h>
#include <WaterValve.h>

namespace gateways {
namespace water_valve {

Gateway& Gateway::get() {
    static Gateway water_valve;
    return water_valve;
}

Gateway::~Gateway() = default;

GatewayID Gateway::getTypeId() const {
    return GatewayID::WaterValve;
}

Gateway::ClbkRet Gateway::updateComponent( UpdContext& ctx ) {
    gateways::water_valve::WaterValveManager::get().onLoop();

    return ClbkRet::Continue;
}

size_t Gateway::getRecommendedThreadStackSize() const {
    return 4096;
}

bool Gateway::initialize() {
    return WaterValve::get().initialize() && WaterValveManager::get().initialize();
}

void Gateway::startup( ScriptExecutor scriptExecutor ) {
    WaterValve::get().startup( scriptExecutor );
}

Gateway::AddingDeviceStatus Gateway::restoreDeviceInfo( const DeviceId&, ::storage::File& ) {
    return AddingDeviceStatus::OK;
}

}  // namespace water_valve
}  // namespace gateways
