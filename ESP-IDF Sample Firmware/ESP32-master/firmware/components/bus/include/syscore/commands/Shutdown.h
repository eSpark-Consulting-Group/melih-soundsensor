#pragma once

#include "BusCommand.h"

namespace core {
namespace commands {

class Shutdown final : public bus::IBusCommand {
public:
    Shutdown() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }
};

}  // namespace commands
}  // namespace core