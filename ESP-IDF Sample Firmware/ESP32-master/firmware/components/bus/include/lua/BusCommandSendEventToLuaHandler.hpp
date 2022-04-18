#pragma once

#include <cassert>
#include <unordered_map>

#include "BusCommand.h"
#include "BusCommandHandleEvent.h"
#include "CommonResources.h"
#include "IValueCast.h"

namespace common {
class IValue;
class TableValue;
}  // namespace common

namespace bus::command {

/**
 * Generic container for Lua events already filtered & sent to a specific Lua handler.
 */
class SendGenericEventToLuaHandler : public HandleEventBase {
public:
    using table_string_to_ivalue = std::unordered_map<std::string, std::unique_ptr<::common::IValue>>;

    SendGenericEventToLuaHandler( std::string handlerName, std::unique_ptr<::common::TableValue> event )
    : HandleEventBase( std::move( handlerName ) )
    , m_event( std::move( event ) ) {}

    SendGenericEventToLuaHandler(
        std::string handlerName,
        const char* eventName,
        std::initializer_list<std::pair<std::string, std::unique_ptr<::common::IValue>>>&& eventFields )
    : HandleEventBase( std::move( handlerName ) )
    , m_event(::common::iValueCast<::common::TableValue>(::common::IValue::create( std::move( eventFields ) ) ) ) {
        m_event->insert(::common::resources::tokens::Event, ::common::IValue::create( eventName ) );
    }

    SendGenericEventToLuaHandler( std::string handlerName, const char* eventName, table_string_to_ivalue&& eventFields )
    : HandleEventBase( std::move( handlerName ) )
    , m_event( std::make_unique<::common::TableValue>() ) {
        for( auto&& field : eventFields ) {
            m_event->insert( field.first, std::move( field.second ) );
        }
        m_event->insert(::common::resources::tokens::Event, ::common::IValue::create( eventName ) );
    }

    const auto& getEvent() const {
        assert( m_event );
        return *m_event;
    }

    void accept(::bus::BusAddr from, ::bus::BusAddr to, ::bus::IBusCommandVisitor& client ) final {
        client.visit( from, to, *this );
    }

private:
    std::unique_ptr<::common::TableValue> m_event;
};

}  // namespace bus::command
