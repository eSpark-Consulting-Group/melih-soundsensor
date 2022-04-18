
#pragma once

#include <NmaCommand.h>

namespace gateways {

class Command : public nma::Command {
public:
    virtual bool executeInTheGateway() = 0;

    virtual bool isNeedToExecuteOnGatewaySide() const {
        return true;
    }

    virtual bool isReadyToSendAnswer() const {
        return true;
    }

    virtual bool hasTimeoutExpired() const {
        return false;
    }
};

}  // namespace gateways
