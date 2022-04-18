#pragma once

#include <BusCommand.h>

namespace bus {
namespace command {

struct ReconstructNetwork final : public IBusCommand {
    virtual ~ReconstructNetwork() = default;

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace command
}  // namespace bus
