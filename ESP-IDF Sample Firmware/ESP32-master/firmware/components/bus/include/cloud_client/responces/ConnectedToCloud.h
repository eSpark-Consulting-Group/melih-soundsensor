#pragma once

#include "BusCommand.h"

namespace cloud_client {
namespace responces {

class ConnectedToCloud : public bus::IBusCommand {
public:
    ConnectedToCloud() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace responces
}  // namespace cloud_client
