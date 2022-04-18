#pragma once

#include <string>
#include <BusCommand.h>

namespace bus {
namespace command {

struct RoomsCreated final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_roomId;
    std::string m_name;
    bool m_edited;
};

}  // namespace command
}  // namespace bus
