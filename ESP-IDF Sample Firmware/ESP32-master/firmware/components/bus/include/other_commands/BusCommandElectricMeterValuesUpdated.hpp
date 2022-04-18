#pragma once

#include "BusCommandHandleEvent.h"

namespace bus {
namespace command {

class ElectricMeterValuesUpdated final : public HandleEvent {
public:
    ElectricMeterValuesUpdated( float voltage,
                                float current,
                                float power,
                                float consumedEnergy,
                                std::string handlerName,
                                std::string event,
                                uint8_t nodeId )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_voltage( voltage )
    , m_current( current )
    , m_power( power )
    , m_consumed_energy( consumedEnergy ) {}

    float voltage() const {
        return m_voltage;
    }

    float current() const {
        return m_current;
    }

    float power() const {
        return m_power;
    }

    float consumedEnergy() const {
        return m_consumed_energy;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    float m_voltage;
    float m_current;
    float m_power;
    float m_consumed_energy;
};

}  // namespace command
}  // namespace bus
