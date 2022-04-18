#include "lua/BusCommandGenericLuaEvent.h"

#include <cassert>

#include "CommonResources.h"
#include "DebugUtils.h"
#include "values/TableValue.h"
#include <CommonResources.h>

namespace bus::lua_event {

const char* GenericLuaEvent::getEventNameStr( EventName eventName ) {
    switch( eventName ) {
    case DeviceUpdated:
        return "device_updated";
    case ConfigurationValueUpdated:
        return "configuration_value_updated";
    case ConfigurationRequestStatusUpdated:
        return "configuration_request_status_updated";
    case ModuleResetUpdate:
        return common::resources::broadcasts::events::ModuleReset;
    default:
        return "unknown";
    }
}

GenericLuaEvent::GenericLuaEvent(
    EventName eventName,
    std::initializer_list<std::pair<std::string, std::unique_ptr<::common::IValue>>>&& eventFields )
: m_event( std::make_unique<::common::TableValue>() ) {
    for( auto&& field : eventFields ) {
        m_event->insert( field.first, field.second->clone() );
    }
    m_event->insert(::common::resources::tokens::Event, ::common::IValue::create( getEventNameStr( eventName ) ) );
}

const ::common::TableValue& GenericLuaEvent::getEvent() const {
    assert( m_event );
    return *m_event;
}

void GenericLuaEvent::accept(::bus::BusAddr from, ::bus::BusAddr to, IBusCommandVisitor& client ) {
    client.visit( from, to, *this );
}

}  // namespace bus::lua_event
