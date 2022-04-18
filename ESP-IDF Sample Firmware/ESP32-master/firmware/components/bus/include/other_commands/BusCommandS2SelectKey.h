#pragma once

#include <BusCommand.h>
#include <string>
#include <vector>

namespace bus {
namespace command {

struct S2SelectKey final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::vector<std::string> m_authModes;
};

}  // namespace command
}  // namespace bus
