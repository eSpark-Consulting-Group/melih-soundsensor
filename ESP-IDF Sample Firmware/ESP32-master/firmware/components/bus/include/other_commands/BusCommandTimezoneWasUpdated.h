#pragma once

#include <BusCommand.h>

namespace bus {
namespace command {

struct TimezoneWasUpdated final : public IBusCommand {
    virtual ~TimezoneWasUpdated() = default;

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
