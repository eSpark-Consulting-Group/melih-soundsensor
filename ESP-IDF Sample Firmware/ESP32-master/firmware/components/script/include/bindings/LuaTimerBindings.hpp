#pragma once

#include "Lua.h"
#include "LuaBinding.hpp"

#include "IValue.h"
#include "IValueCast.h"
#include "values/TableValue.h"

#include <HardUtils.h>

#include <CommonResources.h>
#include "LuaModuleTimers.h"

namespace lua {
namespace module {
namespace bindings {
namespace timer {

#define SIMPLE_TMRBIND_DEBUG
#ifdef SIMPLE_TMRBIND_DEBUG

#include <esp_log.h>
#include "EspLogTags.h"

#define TMRBIND_LOGI( ... ) ESP_LOGI(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )
#define TMRBIND_LOGE( ... ) ESP_LOGE(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )
#define TMRBIND_LOGW( ... ) ESP_LOGW(::esp_log_tags::LUA_TIMER, __VA_ARGS__ )

#else

#define TMRBIND_LOGI( ... ) \
    do {                    \
    } while( false )
#define TMRBIND_LOGE( ... ) \
    do {                    \
    } while( false )
#define TMRBIND_LOGW( ... ) \
    do {                    \
    } while( false )

#endif

class SetTimeout : public LuaBinding<std::string( int, std::string, std::unique_ptr<common::TableValue> )> {
public:
    std::string operator()( int delay, std::string script, std::unique_ptr<::common::TableValue> args ) const {
        TMRBIND_LOGI( "timer::set_timeout( delay=%d, script=%s, args=%s )",
                      delay,
                      script.c_str(),
                      args ? args->toJson().c_str() : "null" );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        if( auto timerId = inst.createTimer( std::move( script ),
                                             std::move( args ),
                                             std::chrono::milliseconds( delay ),
                                             /* isSingleShot */ true ) ) {
            return *timerId;
        }
        return {};
    }
};

class SetInterval : public LuaBinding<std::string( int, std::string, std::unique_ptr<common::TableValue> )> {
public:
    std::string operator()( int delay, std::string script, std::unique_ptr<::common::TableValue> args ) const {
        TMRBIND_LOGI( "timer::set_interval( delay=%d, script=%s, args=%s )",
                      delay,
                      script.c_str(),
                      args ? args->toJson().c_str() : "null" );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        if( auto timerId = inst.createTimer( std::move( script ),
                                             std::move( args ),
                                             std::chrono::milliseconds( delay ),
                                             /* isSingleShot */ false ) ) {
            return *timerId;
        }
        return {};
    }
};

class SetTimeoutWithId : public LuaBinding<void( int, std::string, std::string, std::unique_ptr<common::TableValue> )> {
public:
    void operator()( int delay,
                     std::string timerId,
                     std::string script,
                     std::unique_ptr<::common::TableValue> args ) const {
        TMRBIND_LOGI( "timer::set_timeout_with_id( delay=%d, timerId=%s, script=%s, args=%s )",
                      delay,
                      timerId.c_str(),
                      script.c_str(),
                      args ? args->toJson().c_str() : "null" );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        inst.createTimer( std::move( timerId ),
                          std::move( script ),
                          std::move( args ),
                          std::chrono::milliseconds( delay ),
                          /* isSingleShot */ true );
    }
};

class SetIntervalWithId
: public LuaBinding<void( int, std::string, std::string, std::unique_ptr<common::TableValue> )> {
public:
    void operator()( int delay,
                     std::string timerId,
                     std::string script,
                     std::unique_ptr<::common::TableValue> args ) const {
        TMRBIND_LOGI( "timer::set_interval_with_id( delay=%d, timerId=%s, script=%s, args=%s )",
                      delay,
                      timerId.c_str(),
                      script.c_str(),
                      args ? args->toJson().c_str() : "null" );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        inst.createTimer( std::move( timerId ),
                          std::move( script ),
                          std::move( args ),
                          std::chrono::milliseconds( delay ),
                          /* isSingleShot */ false );
    }
};

class GetArguments : public LuaBinding<std::unique_ptr<common::TableValue>( std::string )> {
public:
    std::unique_ptr<common::TableValue> operator()( std::string timerId ) const {
        TMRBIND_LOGI( "timer::get_arguments( timerId=%s )", timerId.c_str() );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        return inst.getHandlerScriptArgs( timerId );
    }
};

class ResetArguments : public Overloaded<LuaBinding<void( std::string, std::unique_ptr<common::TableValue> )>,
                                         LuaBinding<void( std::string )>> {
public:
    void operator()( std::string timerId, std::unique_ptr<::common::TableValue> args ) const {
        TMRBIND_LOGI(
            "timer::reset_arguments( timerId=%s, args=%s )", timerId.c_str(), args ? args->toJson().c_str() : "null" );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        inst.setHandlerScriptArgs( timerId, std::move( args ) );
    }

    void operator()( std::string timerId ) const {
        TMRBIND_LOGI( "timer::reset_arguments( timerId=%s )", timerId.c_str() );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        inst.setHandlerScriptArgs( timerId, {} );
    }
};

class Cancel : public LuaBinding<void( std::string )> {
public:
    void operator()( std::string timerId ) const {
        TMRBIND_LOGI( "timer::cancel( timerId=%s )", timerId.c_str() );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        inst.removeTimer( timerId );
    }
};

class Exists : public LuaBinding<bool( std::string )> {
public:
    bool operator()( std::string timerId ) const {
        TMRBIND_LOGI( "timer::exists( timerId=%s )", timerId.c_str() );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        return inst.isTimerExist( timerId );
    }
};

class Reset : public LuaBinding<void( int, std::string )> {
public:
    void operator()( int delay, std::string timerId ) const {
        TMRBIND_LOGI( "timer::reset( delay= %d, timerId=%s )", delay, timerId.c_str() );
        Timers& inst = reinterpret_cast<Timers&>( m_ctx->getModule() );
        inst.resetTimer( timerId, std::chrono::milliseconds( delay ) );
    }
};

}  // namespace timer
}  // namespace bindings
}  // namespace module
}  // namespace lua
