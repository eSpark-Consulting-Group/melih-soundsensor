#include "LuaEventManager.h"

#include "IValueCast.h"
#include "BusCommand.h"
#include <BusCommandSetHandler.h>
#include <BusCommandHandleEventUpdateProductName.hpp>
#include <lua/BusCommandGenericLuaEvent.h>
#include <lua/BusCommandSendEventToLuaHandler.hpp>
#include <esp_log.h>
#include "EspLogTags.h"

#define CORE_LOGD( ... ) ESP_LOGD(::esp_log_tags::CORE, __VA_ARGS__ )
#define CORE_LOGI( ... ) ESP_LOGI(::esp_log_tags::CORE, __VA_ARGS__ )
#define CORE_LOGW( ... ) ESP_LOGW(::esp_log_tags::CORE, __VA_ARGS__ )
#define CORE_LOGE( ... ) ESP_LOGE(::esp_log_tags::CORE, __VA_ARGS__ )

namespace lua {

LuaEventManager::LuaEventManager( const std::chrono::seconds& shutdownDelay )
: ParentType( shutdownDelay )
, m_eventsHandler{ bus::BusAddrInvalid, std::string() } {
    CORE_LOGI( "Constructor(...)" );
}

LuaEventManager::~LuaEventManager() {
    CORE_LOGI( "Destructor()" );
}

bool LuaEventManager::initializeImpl() {
    return ParentType::initializeImpl() && bus::Bus::get().join(::bus::lua_event::BUS_NAME, *this );
}

void LuaEventManager::visit( const bus::BusAddr from,
                             const bus::BusAddr to,
                             bus::command::HandleEventUpdateProductName& command ) {
    send<bus::command::HandleEventUpdateProductName>(
        m_eventsHandler.m_addr, m_eventsHandler.m_handlerName, "product_name_updated", 0 );
}

void LuaEventManager::visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::SetHandler& command ) {
    if( command.isSubscribe() ) {
        auto addr = bus::BusAddrInvalid == command.addr() ? from : command.addr();
        m_eventsHandler = { addr, command.handlerName() };
    }
    else {
        m_eventsHandler = { bus::BusAddrInvalid, std::string() };
    }
}

void LuaEventManager::visit(::bus::BusAddr, ::bus::BusAddr, ::bus::lua_event::GenericLuaEvent& command ) {
    auto event = ::common::iValueCast<::common::TableValue>( command.getEvent().clone() );
    assert( event );
    if( event ) {
        send<::bus::command::SendGenericEventToLuaHandler>(
            m_eventsHandler.m_addr, m_eventsHandler.m_handlerName, std::move( event ) );
    }
}

}  // namespace lua
