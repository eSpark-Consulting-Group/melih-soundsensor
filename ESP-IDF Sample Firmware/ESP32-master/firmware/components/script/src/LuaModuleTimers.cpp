#include <cassert>
#include <sstream>
#include <cstdio>

#include "HardUtils.h"
#include "LuaModuleTimers.h"
#include "Lua.h"
#include "bindings/LuaBindingHelper.h"
#include "bindings/LuaTimerBindings.hpp"

#define SIMPLE_MODTMR_DEBUG
#ifdef SIMPLE_MODTMR_DEBUG

#include <esp_log.h>
#include "EspLogTags.h"

#define MODTMR_LOGI( ... ) ESP_LOGI(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )
#define MODTMR_LOGE( ... ) ESP_LOGE(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )
#define MODTMR_LOGW( ... ) ESP_LOGW(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )

#else

#define MODTMR_LOGI( ... ) \
    do {                   \
    } while( false )
#define MODTMR_LOGE( ... ) \
    do {                   \
    } while( false )
#define MODTMR_LOGW( ... ) \
    do {                   \
    } while( false )

#endif

using namespace std::placeholders;

namespace lua {

namespace module {

Timers::Timers( Executor& exec, const char* name )
: IModule( exec, name ) {}

bool Timers::initialize() {
    MODTMR_LOGI( "Module was initialized.\n" );
    return true;
}

int Timers::regModule( lua_State* lua ) {
    static const auto bindArray = lua::createBindingsMap(

        luaL_Reg{ "set_timeout", bindings::luaBinding<bindings::timer::SetTimeout> },
        luaL_Reg{ "set_interval", bindings::luaBinding<bindings::timer::SetInterval> },
        luaL_Reg{ "set_timeout_with_id", bindings::luaBinding<bindings::timer::SetTimeoutWithId> },
        luaL_Reg{ "set_interval_with_id", bindings::luaBinding<bindings::timer::SetIntervalWithId> },
        luaL_Reg{ "get_arguments", bindings::luaBinding<bindings::timer::GetArguments> },
        luaL_Reg{ "reset_arguments", bindings::luaBinding<bindings::timer::ResetArguments> },
        luaL_Reg{ "cancel", bindings::luaBinding<bindings::timer::Cancel> },
        luaL_Reg{ "exists", bindings::luaBinding<bindings::timer::Exists> },
        luaL_Reg{ "reset", bindings::luaBinding<bindings::timer::Reset> } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

void Timers::update() {
    std::set<std::string> triggeredTimerIDs;

    // Collect triggered timer IDs
    for( auto& timerNode : m_timers ) {
        auto elapsed = std::chrono::system_clock::now() - timerNode.second.m_startTime;
        if( elapsed >= timerNode.second.m_duration ) {
            MODTMR_LOGI( "Detected triggered timer [%s]", timerNode.first.c_str() );
            triggeredTimerIDs.insert( timerNode.first );
        }
    }

    // Invoke handlers of triggered timers
    for( auto& timerId : triggeredTimerIDs ) {
        if( auto timerIter = m_timers.find( timerId ); timerIter != m_timers.end() ) {
            timerIter->second.m_startTime = std::chrono::system_clock::now();
            // copy script name and script args because timer handler script may modify m_timers
            std::string scriptName = timerIter->second.m_handlerScriptName;
            auto scriptArgs = timerIter->second.m_isSingleShoot ?
                                  std::move( m_timers.extract( timerIter ).mapped().m_handlerScriptArgs ) :
                                  timerIter->second.m_handlerScriptArgs->clone();
            MODTMR_LOGI( "Invoking timer [%s], script=%s, args=%s",
                         timerId.c_str(),
                         scriptName.c_str(),
                         scriptArgs ? scriptArgs->toJson().c_str() : "null" );
            m_exec.execScript( scriptName, [&scriptArgs]( lua_State* lua ) -> int {
                scriptArgs->accept( ValueToLuaGenerator{ lua } );
                return 1;
            } );
        }
    }
}

std::optional<std::string> Timers::createTimer( std::string handlerScriptName,
                                                std::unique_ptr<::common::TableValue> handlerScriptArgs,
                                                std::chrono::milliseconds duration,
                                                bool isSingleShoot ) {
    auto timerId = generateTimerId();
    return timerId && createTimer( *timerId,
                                   std::move( handlerScriptName ),
                                   std::move( handlerScriptArgs ),
                                   duration,
                                   isSingleShoot ) ?
               timerId :
               std::nullopt;
}

bool Timers::createTimer( std::string timerId,
                          std::string handlerScriptName,
                          std::unique_ptr<::common::TableValue> handlerScriptArgs,
                          std::chrono::milliseconds duration,
                          bool isSingleShoot ) {
    if( !handlerScriptArgs ) {
        handlerScriptArgs = std::make_unique<::common::TableValue>();
    }
    handlerScriptArgs->insert( common::resources::lua_events::TimerId, common::IValue::create( timerId ) );
    auto result = m_timers.try_emplace( std::move( timerId ),
                                        Timer_t{ std::move( handlerScriptName ),
                                                 std::move( handlerScriptArgs ),
                                                 duration,
                                                 isSingleShoot,
                                                 std::chrono::system_clock::now() } );
    return result.second;
}

bool Timers::removeTimer( const std::string& timerId ) {
    return m_timers.erase( timerId ) == 1;
}

std::unique_ptr<::common::TableValue> Timers::getHandlerScriptArgs( const std::string& timerId ) const {
    if( auto timerIter = m_timers.find( timerId );
        timerIter != m_timers.end() && timerIter->second.m_handlerScriptArgs ) {
        return ::common::iValueCast<::common::TableValue>( timerIter->second.m_handlerScriptArgs->clone() );
    }
    return {};
}

bool Timers::setHandlerScriptArgs( const std::string& timerId, std::unique_ptr<::common::TableValue> args ) {
    if( auto timerIter = m_timers.find( timerId ); timerIter != m_timers.end() ) {
        if( !args ) {
            args = std::make_unique<::common::TableValue>();
        }
        args->insert( common::resources::lua_events::TimerId, common::IValue::create( timerId ) );
        timerIter->second.m_handlerScriptArgs = std::move( args );
        return true;
    }
    return false;
}

bool Timers::resetTimer( const std::string& timerId, std::chrono::milliseconds duration ) {
    if( auto timer = m_timers.find( timerId ); timer != m_timers.end() ) {
        timer->second.m_duration = duration;
        timer->second.m_startTime = std::chrono::system_clock::now();
        return true;
    }
    return false;
}

bool Timers::isTimerExist( const std::string& timerId ) const {
    return m_timers.count( timerId );
}

std::optional<std::string> Timers::generateTimerId() const {
    char timerId[64];
    if( snprintf( timerId, sizeof( timerId ), "tmr_%X", hard::common::generateUniqueId() ) == 0 ) {
        MODTMR_LOGE( "Cannot generate a new timer ID" );
        return std::nullopt;
    }
    if( isTimerExist( timerId ) ) {
        MODTMR_LOGE( "Cannot generate timer ID. Timer [%s] already exists.", timerId );
        return std::nullopt;
    }
    return timerId;
}

}  // end of namespace module

}  // end of namespace lua
