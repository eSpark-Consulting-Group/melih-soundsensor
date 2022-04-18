#pragma once

#include "BusCommandHandleEvent.h"

namespace bus {
namespace command {

class WaterLeakValueUpdate final : public HandleEvent {
public:
    WaterLeakValueUpdate( bool value, uint8_t sensorIndex, std::string handlerName, std::string event, uint8_t nodeId )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_value( value )
    , m_sensor_index( sensorIndex ) {}

    bool value() const {
        return m_value;
    }

    uint8_t sensorIndex() const {
        return m_sensor_index;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    bool m_value;
    uint8_t m_sensor_index;
};

}  // namespace command
}  // namespace bus
