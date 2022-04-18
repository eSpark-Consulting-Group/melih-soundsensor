#pragma once

#include <string>
#include <BusCommand.h>

namespace bus {
namespace command {

struct ModesSwitched final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) {
        client.visit( from, to, *this );
    }

    std::string m_from;
    std::string m_to;
    std::string m_status;
    int m_switchDelay = 0;
};

}  // namespace command
}  // namespace bus
