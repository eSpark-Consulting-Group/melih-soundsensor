#pragma once

#if CONFIG_REMOTE_LOGGING

#include "LuaBinding.hpp"

#include "values/TableValue.h"
#include "values/ArrayValue.h"
#include "values/IntValue.h"
#include "IValueCast.h"
#include "IGatewayUserDataStorage.h"

#include "Lua.h"
#include <DebugUtils.h>
#include "RemoteLogger.h"

namespace lua {
namespace module {
namespace bindings {
namespace logging {

std::string generateMessage( const std::string& value, const std::string& severity, const std::string& pluginName ) {
    std::string message;
    auto now = std::chrono::system_clock::now();
    std::time_t tp = std::chrono::system_clock::to_time_t( now );
    constexpr auto date_buf_size = 32;
    char date[date_buf_size];
    std::strftime( date, date_buf_size, "%Y-%m-%d %H:%M:%S", std::localtime( &tp ) );

    // microseconds part
    auto now_floor = std::chrono::system_clock::from_time_t( tp );
    uint32_t us = std::chrono::duration_cast<std::chrono::microseconds>( now - now_floor ).count();
    snprintf( date + strlen( date ), date_buf_size - strlen( date ), ".%d", us );
    //
    message += date;
    message += " trace : ";
    message += severity;
    message += " : " + pluginName;
    message += " : " + value;
    return message;
}

class GetLevel : public LuaBinding<std::string( void )> {
public:
    std::string operator()() {
        return core::Logger::LevelToText( ( core::Logger::get()->getLogLevel() ) );
    }
};

class Log : public LuaBinding<void( std::string, std::unique_ptr<common::IValue> )> {
public:
    void operator()( std::string severity, std::unique_ptr<common::IValue> value ) {
        if( 0 == strcmp( severity.c_str(), "trace" ) ) {
            std::string message = generateMessage( value->toJson(), "trace", m_ctx->getPluginId() );
            ESP_LOGV(::esp_log_tags::LUA, "%s", message.c_str() );
        }
        else if( 0 == strcmp( severity.c_str(), "debug" ) ) {
            std::string message = generateMessage( value->toJson(), "debug", m_ctx->getPluginId() );
            ESP_LOGD(::esp_log_tags::LUA, "%s", message.c_str() );
        }
        else if( 0 == strcmp( severity.c_str(), "info" ) ) {
            std::string message = generateMessage( value->toJson(), "info", m_ctx->getPluginId() );
            ESP_LOGI(::esp_log_tags::LUA, "%s", message.c_str() );
        }
        else if( 0 == strcmp( severity.c_str(), "warning" ) ) {
            std::string message = generateMessage( value->toJson(), "warning", m_ctx->getPluginId() );
            ESP_LOGW(::esp_log_tags::LUA, "%s", message.c_str() );
        }
        else if( 0 == strcmp( severity.c_str(), "error" ) ) {
            std::string message = generateMessage( value->toJson(), "error", m_ctx->getPluginId() );
            ESP_LOGE(::esp_log_tags::LUA, "%s", message.c_str() );
        }
    }
};

class Trace : public LuaBinding<void( std::unique_ptr<common::IValue> )> {
public:
    void operator()( std::unique_ptr<common::IValue> value ) {
        std::string message = generateMessage( value->toJson(), "trace", m_ctx->getPluginId() );
        ESP_LOGV(::esp_log_tags::LUA, "%s", message.c_str() );
    }
};

class Debug : public LuaBinding<void( std::unique_ptr<common::IValue> )> {
public:
    void operator()( std::unique_ptr<common::IValue> value ) {
        std::string message = generateMessage( value->toJson(), "debug", m_ctx->getPluginId() );
        ESP_LOGD(::esp_log_tags::LUA, "%s", message.c_str() );
    }
};

class Info : public LuaBinding<void( std::unique_ptr<common::IValue> )> {
public:
    void operator()( std::unique_ptr<common::IValue> value ) {
        std::string message = generateMessage( value->toJson(), "info", m_ctx->getPluginId() );
        ESP_LOGI(::esp_log_tags::LUA, "%s", message.c_str() );
    }
};

class Warning : public LuaBinding<void( std::unique_ptr<common::IValue> )> {
public:
    void operator()( std::unique_ptr<common::IValue> value ) {
        std::string message = generateMessage( value->toJson(), "warning", m_ctx->getPluginId() );
        ESP_LOGW(::esp_log_tags::LUA, "%s", message.c_str() );
    }
};

class Error : public LuaBinding<void( std::unique_ptr<common::IValue> )> {
public:
    void operator()( std::unique_ptr<common::IValue> value ) {
        std::string message = generateMessage( value->toJson(), "error", m_ctx->getPluginId() );
        ESP_LOGE(::esp_log_tags::LUA, "%s", message.c_str() );
    }
};

}  // namespace logging
}  // namespace bindings
}  // namespace module
}  // namespace lua

#endif  // CONFIG_REMOTE_LOGGING
