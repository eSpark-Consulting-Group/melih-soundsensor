#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

struct StartExclude final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_nodeId;
};

}  // namespace command
}  // namespace bus
