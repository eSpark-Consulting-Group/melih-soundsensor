#pragma once

#include <string>
#include <map>
#include <BusCommandHandleEvent.h>

namespace cloud {
namespace responses {

class HandleEventCloudDeviceUpdated final : public bus::command::HandleEvent {
public:
    HandleEventCloudDeviceUpdated( std::string&& id,
                                   std::map<std::string, std::string>&& m_fields,
                                   std::string handlerName,
                                   std::string event,
                                   uint8_t nodeId )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_Id( std::move( id ) )
    , m_fields( std::move( m_fields ) ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline auto id() const {
        return m_Id;
    }

    inline auto fields() const {
        return m_fields;
    }

private:
    std::string m_Id;
    std::map<std::string, std::string> m_fields;
};

}  // namespace responses
}  // namespace cloud
