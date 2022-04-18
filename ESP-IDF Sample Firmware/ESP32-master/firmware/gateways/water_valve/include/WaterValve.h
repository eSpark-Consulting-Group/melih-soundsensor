
#pragma once

#include <Bus.h>
#include <BusCommandSetHandler.h>

#include <CommonResources.h>
#include <IGateway.h>
#include "BusEventsHandler.h"

namespace gateways {
namespace water_valve {

class WaterValve : public bus::IBusClient, public common::IInitialize {
    static constexpr auto BUS_CLIENT_NAME = "watervalve";
    static constexpr int BUS_WAIT_TIME_MS = 0;
    static constexpr int ITEMS_ADD_DELAY_BRODCAST = 100;

public:
    static WaterValve& get();

    bool initialize_hw();

    void startup(::gateways::IGateway::ScriptExecutor );

public:
    bool onLoop();
    void onUpdateValveState( const bool value );
    void onUpdateWaterLeak( const bool value, const uint8_t sensor_index );

private:
    WaterValve();

    bool initializeImpl() override;

    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::RequestAllValue& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SwitchValueSet& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::GpioButtonClicked& command ) override;

private:
    bus::EventsHandler m_eventsHandler;
};

}  // namespace water_valve
}  // namespace gateways
