#pragma once

#include <BusCommand.h>
#include <chrono>

namespace bus {
namespace command {

struct RequestValue : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    unsigned int m_nodeId;
    unsigned int m_commandClassId;
    unsigned int m_channelId;
    std::chrono::milliseconds m_requestDelay;
};

}  // namespace command
}  // namespace bus
