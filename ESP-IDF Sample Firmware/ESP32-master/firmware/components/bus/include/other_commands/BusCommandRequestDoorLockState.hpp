#pragma once

#include "BusCommandDoorLockRequest.hpp"

namespace bus::command {

class RequestDoorLockState : public ::bus::command::DoorLockRequest {
public:
    using DoorLockRequest::DoorLockRequest;

    void accept(::bus::BusAddr from, ::bus::BusAddr to, ::bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }
};

}  // namespace bus::command
