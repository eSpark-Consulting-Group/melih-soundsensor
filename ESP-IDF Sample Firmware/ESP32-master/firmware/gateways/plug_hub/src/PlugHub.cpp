
#include <cassert>
#include <PlugHub.h>
#include <PlugHubManager.h>
#include <GatewayID.h>
#include <NotificationEvent.h>

#include <DebugUtils.h>
#include "sdkconfig.h"

#include <BusCommandSetHandler.h>
#include <BusCommandPluginRun.h>
#include <BusCommandUpdateGateway.h>
#include <BusCommandSwitchValueUpdated.hpp>
#include <BusCommandSwitchValueSet.hpp>
#include <BusCommandSetConsumedEnergyMeter.hpp>
#include <BusCommandElectricMeterValuesUpdated.hpp>
#include <BusCommandResetEnergyMeter.hpp>
#include <BusCommandGpioButton.hpp>
#include <Meter_cc.h>

#include <Ezlo.h>
#include <plugin_manager.h>

namespace gateways {
namespace plug_hub {

PlugHub::PlugHub()
: m_eventsHandler( { 0, "" } ) {
    if( !initialize() ) {
        FUNC_LOGE( "PluginHub bus's client has not been initialized" );
    }
}

PlugHub& PlugHub::get() {
    static PlugHub plughub;
    return plughub;
}

bool PlugHub::initializeImpl() {
    return bus::Bus::get().join( BUS_CLIENT_NAME, *this ) &&
           bus::Bus::get().join(::bus::command::GPIO_BUTTON_BUS_CLIENT_NAME, *this );
}

void PlugHub::startup( gateways::IGateway::ScriptExecutor scriptExecutor ) {
    auto& pluginManager = plugin::PluginManager::get();
    auto pConf = pluginManager.getPluginConfig( "plughub" );
    const char* name;
    name = pConf.json( "migration" ).toString();
    if( name ) {
        auto script_name = std::string( "plughub/" ) + name + std::string( ".lua" );
        scriptExecutor( script_name );
    }
    name = pConf.json( "startup" ).toString();
    if( name ) {
        auto script_name = std::string( "plughub/" ) + name + std::string( ".lua" );
        scriptExecutor( script_name );
    }
}

bool PlugHub::onLoop() {
    tryReceive( BUS_WAIT_TIME_MS / portTICK_PERIOD_MS );
    return true;
}

void PlugHub::onUpdatePlugState( const bool value ) {
    if( !m_eventsHandler.m_handlerName.empty() && m_eventsHandler.m_addr != 0 ) {
        send( m_eventsHandler.m_addr,
              std::make_unique<bus::command::SwitchValueUpdated>(
                  value, m_eventsHandler.m_handlerName, "switch_value_updated", 0 ) );
    }
}

void PlugHub::onUpdateMeterValue( const float voltage,
                                  const float current,
                                  const float power,
                                  const float consumed_energy ) {
    if( !m_eventsHandler.m_handlerName.empty() && m_eventsHandler.m_addr != 0 ) {
        FUNC_LOGI( "nMeterDATA: U:%f A:%f P:%f KW_h:%f", voltage, current, power, consumed_energy );
        send( m_eventsHandler.m_addr,
              std::make_unique<bus::command::ElectricMeterValuesUpdated>( voltage,
                                                                          current,
                                                                          power,
                                                                          consumed_energy,
                                                                          m_eventsHandler.m_handlerName,
                                                                          "electric_meter_values_updated",
                                                                          0 ) );
    }
}

void PlugHub::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) {
    if( command.isSubscribe() ) {
        m_eventsHandler = { from, command.handlerName() };
    }
    else {
        m_eventsHandler = { bus::BusAddrInvalid, "" };
    }
}

void PlugHub::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RequestAllValue& command ) {
    onUpdatePlugState( PlugHubManager::get().getCurrrentPlugState() );
}

void PlugHub::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueSet& command ) {
    PlugHubManager::get().onUpdatePlugState( command.value() );
}

void PlugHub::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetConsumedEnergyMeter& command ) {
    FUNC_LOGI( "SetConsumedEnergyMeter %f", command.value() );
    Meter_cc::Meter_set_consumed_energy_meter( command.value() );
}

void PlugHub::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ResetEnergyMeter& command ) {
    Meter_cc::Meter_reset();
}

void PlugHub::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::GpioButtonClicked& command ) {
    gateways::plug_hub::PlugHubManager::get().onPressButton();
}

}  // namespace plug_hub
}  // namespace gateways
