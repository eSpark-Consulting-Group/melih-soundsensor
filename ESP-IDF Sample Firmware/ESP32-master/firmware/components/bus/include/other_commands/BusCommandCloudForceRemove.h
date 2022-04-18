#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

struct CloudForceRemove final : public IBusCommand {
    virtual ~CloudForceRemove() = default;

    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string deviceId;
};

}  // namespace command
}  // namespace bus
