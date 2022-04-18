#pragma once

#include <BusCommandHandleEvent.h>

namespace bus {
namespace command {

class ResetEnergyMeter final : public HandleEvent {
public:
    using HandleEvent::HandleEvent;
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
