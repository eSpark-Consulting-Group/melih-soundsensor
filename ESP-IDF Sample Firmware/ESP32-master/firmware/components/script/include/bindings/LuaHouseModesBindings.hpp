#pragma once

#include "LuaBinding.hpp"

#include "BusCommandSetHandler.h"

#include "Lua.h"

#include "HouseModesManager.h"

static constexpr auto HOUSE_MODES_NAME = "znet";

using namespace std::chrono_literals;

namespace lua {
namespace module {
namespace bindings {
namespace house_modes {

class Switch : public LuaBinding<void( std::string )> {
public:
    bool operator()( const std::string& id ) const {
        FUNC_LOGD( "house_modes::switch( id = %s )", id.c_str() );
        return core::HouseModesManager::get().switchPreset( id );
    }
};

class CancelSwitch : public LuaBinding<void()> {
public:
    std::string operator()() const {
        FUNC_LOGD( "house_modes::cancel_switch" );
        return core::HouseModesManager::get().cancelSwitchPreset();
    }
};

class GetCurrent : public LuaBinding<void()> {
public:
    std::string operator()() const {
        FUNC_LOGD( "house_modes::get_current" );
        return core::HouseModesManager::get().currentPreset();
    }
};

class GetModes : public LuaBinding<std::vector<std::string>()> {
public:
    std::vector<std::string> operator()() const {
        FUNC_LOGD( "house_modes::get_modes" );
        std::vector<std::string> result;
        core::HouseModesManager::get().getPresetsIds( result );
        return result;
    }
};

class GetMode : public LuaBinding<std::shared_ptr<::common::TableValue>( std::string )> {
public:
    std::shared_ptr<::common::TableValue> operator()( const std::string& id ) const {
        FUNC_LOGD( "house_modes::get_mode( id = %s )", id.c_str() );
        std::shared_ptr<::common::TableValue> value = std::make_shared<::common::TableValue>();
        auto info = core::HouseModesManager::get().getPresetInfo( id );

        value->insert( "id", std::make_unique<::common::StringValue>( info.id ) );
        value->insert( "name", std::make_unique<::common::StringValue>( info.name ) );
        value->insert( "defaultState", std::make_unique<::common::BoolValue>( info.defaultState ) );

        std::unique_ptr<::common::ArrayValue> disarmed_devices = std::make_unique<::common::ArrayValue>();
        for( auto deviceId : info.disarmed_devices ) {
            disarmed_devices->push_back( std::make_unique<::common::StringValue>( deviceId ) );
        }
        value->insert( "disarmed_devices", std::move( disarmed_devices ) );

        std::unique_ptr<::common::ArrayValue> alarms_off_devices = std::make_unique<::common::ArrayValue>();
        for( auto deviceId : info.alarms_off_devices ) {
            alarms_off_devices->push_back( std::make_unique<::common::StringValue>( deviceId ) );
        }
        value->insert( "alarms_off_devices", std::move( alarms_off_devices ) );

        return value;
    }
};

class FindModeByName : public LuaBinding<std::shared_ptr<::common::TableValue>( std::string )> {
public:
    std::shared_ptr<::common::TableValue> operator()( const std::string& modename ) const {
        FUNC_LOGD( "house_modes::find_mode_by_name( modename = %s )", modename.c_str() );
        std::shared_ptr<::common::TableValue> value = std::make_shared<::common::TableValue>();
        auto info = core::HouseModesManager::get().getPresetInfo( modename );

        value->insert( "id", std::make_unique<::common::StringValue>( info.id ) );
        value->insert( "name", std::make_unique<::common::StringValue>( info.name ) );
        value->insert( "defaultState", std::make_unique<::common::BoolValue>( info.defaultState ) );

        std::unique_ptr<::common::ArrayValue> disarmed_devices = std::make_unique<::common::ArrayValue>();
        for( auto deviceId : info.disarmed_devices ) {
            disarmed_devices->push_back( std::make_unique<::common::StringValue>( deviceId ) );
        }
        value->insert( "disarmed_devices", std::move( disarmed_devices ) );

        std::unique_ptr<::common::ArrayValue> alarms_off_devices = std::make_unique<::common::ArrayValue>();
        for( auto deviceId : info.alarms_off_devices ) {
            alarms_off_devices->push_back( std::make_unique<::common::StringValue>( deviceId ) );
        }
        value->insert( "alarms_off_devices", std::move( alarms_off_devices ) );

        return value;
    }
};

class Subscribe : public LuaBinding<void( std::string )> {
public:
    std::string operator()( std::string scriptName ) const {
        FUNC_LOGD( "house_modes::subscribe( scriptName=%s )", scriptName.c_str() );
        m_ctx->getExecutor().send<bus::command::SetHandler>(
            ::bus::lua_event::BUS_NAME, m_ctx->getExecutor().addr(), scriptName );
        return "exclusive_housemode_subscriber";
    }
};

}  // namespace house_modes
}  // namespace bindings
}  // namespace module
}  // namespace lua
