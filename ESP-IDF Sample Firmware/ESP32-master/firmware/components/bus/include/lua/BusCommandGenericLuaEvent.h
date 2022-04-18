#pragma once

#include "BusCommand.h"

namespace common {
class IValue;
class TableValue;
}  // namespace common

namespace bus::lua_event {

/**
 * Generic container for system events to be filtered & sent to a specific Lua handler.
 * This is an alternative to HandleEvent that delegates selection of a specific Lua handler to LuaEventManager.
 * The use of this class also eliminates the need to define a visitor at ::lua::Executor for each new event, because
 * LuaEventManager convert GenericLuaEvent to another kind of generic event: LuaHandlerEvent.
 */
class GenericLuaEvent : public IBusCommand {
public:
    enum EventName { DeviceUpdated, ConfigurationValueUpdated, ConfigurationRequestStatusUpdated, ModuleResetUpdate };

    static const char* getEventNameStr( EventName );

    GenericLuaEvent( EventName,
                     std::initializer_list<std::pair<std::string, std::unique_ptr<::common::IValue>>>&& eventFields );

    const ::common::TableValue& getEvent() const;

    void accept(::bus::BusAddr from, ::bus::BusAddr to, ::bus::IBusCommandVisitor& client ) final;

private:
    std::unique_ptr<::common::TableValue> m_event;
};

}  // namespace bus::lua_event
