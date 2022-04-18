
#pragma once

#include <Bus.h>
#include <BusCommandSetHandler.h>

#include <CommonResources.h>
#include <IGateway.h>
#include "BusEventsHandler.h"

namespace gateways {
namespace plug_hub {

class PlugHub : public bus::IBusClient, public common::IInitialize {
    static constexpr auto BUS_CLIENT_NAME = "plughub";
    static constexpr int BUS_WAIT_TIME_MS = 0;
    static constexpr int ITEMS_ADD_DELAY_BRODCAST = 100;

public:
    static PlugHub& get();

    void startup( gateways::IGateway::ScriptExecutor scriptExecutor );

public:
    bool onLoop();
    void onUpdatePlugState( const bool value );
    void onUpdateMeterValue( const float voltage, const float current, const float power, const float consumed_energy );

protected:
    PlugHub();

private:
    bool initializeImpl() override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RequestAllValue& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueSet& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::SetConsumedEnergyMeter& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::ResetEnergyMeter& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::GpioButtonClicked& command ) override;

private:
    bus::EventsHandler m_eventsHandler;
};

}  // namespace plug_hub
}  // namespace gateways
