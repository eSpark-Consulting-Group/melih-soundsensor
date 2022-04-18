#pragma once

#include <string>
#include <BusCommand.h>

namespace bus {
namespace command {

struct ModesNotificationsRemoved final : public IBusCommand {
    void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_modeId;
    std::string m_notification;  // userId
};

}  // namespace command
}  // namespace bus
