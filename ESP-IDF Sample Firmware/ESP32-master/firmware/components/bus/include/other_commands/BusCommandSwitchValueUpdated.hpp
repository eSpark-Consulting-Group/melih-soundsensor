#pragma once

#include "BusCommandHandleEvent.h"

namespace bus {
namespace command {

class SwitchValueUpdated final : public HandleEvent {
public:
    SwitchValueUpdated( bool value, std::string handlerName, std::string event, uint8_t nodeId )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_value( value ) {}

    bool value() const {
        return m_value;
    }

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

private:
    bool m_value;
};

}  // namespace command
}  // namespace bus
