
#include <cassert>
#include <WaterValve.h>
#include <WaterValveManager.h>
#include <GatewayID.h>
#include <NotificationEvent.h>

#include <DebugUtils.h>
#include "sdkconfig.h"

#include <BusCommandSetHandler.h>
#include <BusCommandPluginRun.h>
#include <BusCommandUpdateGateway.h>
#include <BusCommandSwitchValueUpdated.hpp>
#include <BusCommandSwitchValueSet.hpp>
#include <BusCommandWaterLeakValueUpdate.hpp>
#include <BusCommandGpioButton.hpp>
#include <Meter_cc.h>
#include <Ezlo.h>
#include <plugin_manager.h>
#include "WaterValve_hard.h"

namespace gateways {
namespace water_valve {

WaterValve::WaterValve()
: m_eventsHandler( { 0, "" } ) {}

WaterValve& WaterValve::get() {
    static WaterValve watervalve;
    return watervalve;
}

bool WaterValve::initialize_hw() {
    WaterValve_hard::get().init_water();
    return true;
}
bool WaterValve::initializeImpl() {
    return bus::Bus::get().join( BUS_CLIENT_NAME, *this ) &&
           bus::Bus::get().join(::bus::command::GPIO_BUTTON_BUS_CLIENT_NAME, *this );
}

void WaterValve::startup(::gateways::IGateway::ScriptExecutor scriptExecutor ) {
    auto& pluginManager = plugin::PluginManager::get();
    auto pConf = pluginManager.getPluginConfig( "water_valve" );
    const char* name = pConf.json( "startup" ).toString();
    if( name ) {
        auto script_name = std::string( "water_valve/" ) + name + std::string( ".lua" );
        scriptExecutor( script_name );
    }
}

bool WaterValve::onLoop() {
    tryReceive( BUS_WAIT_TIME_MS / portTICK_PERIOD_MS );
    return true;
}

void WaterValve::onUpdateValveState( const bool value ) {
    if( !m_eventsHandler.m_handlerName.empty() && m_eventsHandler.m_addr != 0 ) {
        send( m_eventsHandler.m_addr,
              std::make_unique<bus::command::SwitchValueUpdated>(
                  value, m_eventsHandler.m_handlerName, "valve_switch_value_updated", 0 ) );
    }
}

void WaterValve::onUpdateWaterLeak( const bool value, const uint8_t sensor_index ) {
    if( !m_eventsHandler.m_handlerName.empty() && m_eventsHandler.m_addr != 0 ) {
        FUNC_LOGD( "send    bus::command::water_leak_value_updated" );
        send( m_eventsHandler.m_addr,
              std::make_unique<bus::command::WaterLeakValueUpdate>(
                  value, sensor_index, m_eventsHandler.m_handlerName, "water_leak_value_updated", 0 ) );
    }
}

void WaterValve::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) {
    if( command.isSubscribe() ) {
        m_eventsHandler = { from, command.handlerName() };
    }
    else {
        m_eventsHandler = { bus::BusAddrInvalid, "" };
    }
}

void WaterValve::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RequestAllValue& command ) {
    onUpdateValveState( WaterValveManager::get().getCurrrentValveState() );
    onUpdateWaterLeak( 0, 0 );
}

void WaterValve::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueSet& command ) {
    WaterValveManager::get().onUpdateValveState( command.value() );
    printf( "\r\nNew WATER STATE\r\n" );
    WaterValve_hard::get().set_valve( command.value() );
}

void WaterValve::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::GpioButtonClicked& command ) {
    WaterValveManager::get().onPressButton();
}

}  // namespace water_valve
}  // namespace gateways
