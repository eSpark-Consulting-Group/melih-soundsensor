#pragma once

#include <BusCommand.h>
#include <string>
#include <list>

namespace bus {
namespace command {

struct RoomsReordered final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_from;
    std::string m_to;
    std::string m_status;
    std::list<std::string> m_reorderedRooms;
};

}  // namespace command
}  // namespace bus
