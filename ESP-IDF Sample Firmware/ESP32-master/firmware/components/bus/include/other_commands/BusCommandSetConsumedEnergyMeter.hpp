#pragma once

#include "BusCommandHandleEvent.h"

namespace bus {
namespace command {

class SetConsumedEnergyMeter final : public HandleEvent {
public:
    SetConsumedEnergyMeter( float kWt, std::string handlerName, std::string event, uint8_t nodeId )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_kWt( kWt ) {}

    float value() const {
        return m_kWt;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    float m_kWt;
};

}  // namespace command
}  // namespace bus
