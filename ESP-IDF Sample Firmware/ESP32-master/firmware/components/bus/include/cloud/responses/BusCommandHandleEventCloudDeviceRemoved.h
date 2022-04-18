#pragma once

#include <string>
#include <BusCommandHandleEvent.h>

namespace cloud {
namespace responses {

class HandleEventCloudDeviceRemoved final : public bus::command::HandleEvent {
public:
    HandleEventCloudDeviceRemoved( std::string&& id, std::string handlerName, std::string event, uint8_t nodeId )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_Id( std::move( id ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline auto id() const {
        return m_Id;
    }

private:
    std::string m_Id;
};

}  // namespace responses
}  // namespace cloud
