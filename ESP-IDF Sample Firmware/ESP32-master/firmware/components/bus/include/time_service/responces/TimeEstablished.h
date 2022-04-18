#pragma once

#include "BusCommand.h"

namespace time_service {
namespace responce {

class TimeEstablished : public bus::IBusCommand {
public:
    TimeEstablished() = default;

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace responce
}  // namespace time_service
