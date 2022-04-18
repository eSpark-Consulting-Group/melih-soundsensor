#pragma once

#include <string>
#include <BusCommandHandleEvent.h>

namespace cloud {
namespace responses {

class HandleEventCloudResponseCommand final : public bus::command::HandleEvent {
public:
    HandleEventCloudResponseCommand( std::string&& commandId,
                                     std::string&& result,
                                     std::string&& error,
                                     std::string handlerName,
                                     std::string event,
                                     uint8_t nodeId )
    : HandleEvent( std::move( handlerName ), std::move( event ), nodeId )
    , m_commandId( std::move( commandId ) )
    , m_result( std::move( result ) )
    , m_error( error ) {}

    void accept( const bus::BusAddr from, const bus::BusAddr to, bus::IBusCommandVisitor& client ) override {
        client.visit( from, to, *this );
    }

    inline auto commandId() const {
        return m_commandId;
    }

    inline auto result() const {
        return m_result;
    }

    inline auto error() const {
        return m_error;
    }

private:
    std::string m_commandId;
    std::string m_result;
    std::string m_error;
};

}  // namespace responses
}  // namespace cloud
