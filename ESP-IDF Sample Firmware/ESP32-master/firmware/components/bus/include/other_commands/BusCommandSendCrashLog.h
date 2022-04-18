#pragma once

#include <BusCommand.h>

namespace bus {
namespace command {

struct SendCrashLog final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
