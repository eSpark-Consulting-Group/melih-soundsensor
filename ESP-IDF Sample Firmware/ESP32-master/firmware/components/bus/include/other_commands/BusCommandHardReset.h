#pragma once

#include <BusCommand.h>

namespace bus {
namespace command {

struct HardReset final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
