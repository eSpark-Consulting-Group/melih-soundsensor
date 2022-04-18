#include <WaterValve.h>
#include <WaterValveGateway.h>
#include <WaterValveManager.h>

#include <DebugUtils.h>

namespace gateways {
namespace water_valve {

bool WaterValveManager::initialize() {
    // TODO: init port uses for water valve
    return true;
}

WaterValveManager& WaterValveManager::get() {
    static WaterValveManager water_valve_manager;
    return water_valve_manager;
}

bool WaterValveManager::onLoop() {
    WaterValve::get().onLoop();

    const auto now = std::chrono::steady_clock::now();
    const auto elapsedTime = now - m_lastDataUpdateTime;
    if( elapsedTime > UpdateWaterValveValue ) {
        // TODO: update water valve state
        // WaterValve::get().onUpdateWaterLeak( 0, 0 );
        m_lastDataUpdateTime = now;
    }
    return true;
}

bool WaterValveManager::getCurrrentValveState() {
    return m_valve_state;
}

void WaterValveManager::onUpdateValveState( const bool value ) {
    // TODO: change pin state
    m_valve_state = value;
    FUNC_LOGD( "WaterValveManager::onUpdateValveState  %d ", m_valve_state );

    WaterValve::get().onUpdateValveState( value );
}

void WaterValveManager::onPressButton() {
    // m_valve_state = m_valve_state ? 0 : 1;
    onUpdateValveState( m_valve_state );
}

}  // namespace water_valve
}  // namespace gateways
