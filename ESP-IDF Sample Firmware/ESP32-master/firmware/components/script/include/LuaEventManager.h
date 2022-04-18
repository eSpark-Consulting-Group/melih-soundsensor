#pragma once

#include "Shutdownable.h"
#include "BusEventsHandler.h"

namespace lua {

class LuaEventManager : public core::Shutdownable {
public:
    using ParentType = core::Shutdownable;
    using ClbkRet = ParentType::ClbkRet;

    LuaEventManager( const std::chrono::seconds& shutdownDelay );

    ~LuaEventManager();

protected:
    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override {
        return ClbkRet::Continue;
    }

    // IBusCommandVisitor
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) override;

    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::HandleEventUpdateProductName& command ) override;

    void visit(::bus::BusAddr, ::bus::BusAddr, ::bus::lua_event::GenericLuaEvent& command ) override;

private:
    bus::EventsHandler m_eventsHandler;
};

}  // namespace lua
