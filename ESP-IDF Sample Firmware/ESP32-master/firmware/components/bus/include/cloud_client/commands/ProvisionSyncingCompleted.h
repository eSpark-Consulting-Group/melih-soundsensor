#pragma once

#include "BusCommand.h"

#include <string>

namespace cloud_client {
namespace commands {

class ProvisionSyncingCompleted : public bus::IBusCommand {
public:
    ProvisionSyncingCompleted() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace commands
}  // namespace cloud_client
