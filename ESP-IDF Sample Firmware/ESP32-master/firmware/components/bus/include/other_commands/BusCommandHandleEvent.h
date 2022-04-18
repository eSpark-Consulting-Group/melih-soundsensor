#pragma once

#include <BusCommand.h>
#include <string>

namespace bus {
namespace command {

/**
 * Base class for bus commands that we send to ::lua::module::ZWave to invoke some Lua event.
 * Use GenericLuaEvent instead to delegate selection of a Lua handler to LuaEventManager (correct way).
 */
class HandleEventBase : public IBusCommand {
public:
    explicit HandleEventBase( std::string handlerName )
    : m_handlerName( std::move( handlerName ) ) {}

    std::string handlerName() const {
        return m_handlerName;
    }

    void handlerName( std::string newName ) {
        m_handlerName = std::move( newName );
    }

private:
    std::string m_handlerName;
};

/**
 * Extends HandleEventBase with some z-wave-specific requirements to event interface: adds event name and nodeId.
 * Use GenericLuaEvent instead to delegate selection of a Lua handler to LuaEventManager (correct way).
 */
class HandleEvent : public HandleEventBase {
public:
    HandleEvent( std::string handlerName, std::string event, uint8_t nodeId )
    : HandleEventBase( std::move( handlerName ) )
    , m_event( std::move( event ) )
    , m_nodeId( nodeId ) {}

    virtual ~HandleEvent() = default;

    std::string event() const {
        return m_event;
    }

    uint8_t nodeId() const {
        return m_nodeId;
    }

private:
    std::string m_event;
    uint8_t m_nodeId;
};

}  // namespace command
}  // namespace bus
