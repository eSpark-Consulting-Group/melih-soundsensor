#pragma once

#include <BusCommand.h>
#include <string>
#include <vector>

namespace lua {
namespace responce {

struct HubSendBroadcast final : public bus::IBusCommand {
    HubSendBroadcast() {
        m_value.reserve( 6 );
    }

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    std::string m_plugin;
    std::string m_event;
    std::string m_key;
    std::vector<std::string> m_value;
};

}  // namespace responce
}  // namespace lua
