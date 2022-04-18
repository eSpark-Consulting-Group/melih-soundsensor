#pragma once

#include "LuaBinding.hpp"

#include "Lua.h"
#include "ZWaveGateway.h"
#include <CommonResources.h>

static constexpr auto SCENES_NAME = "scenes";

namespace lua {
namespace module {
namespace bindings {
namespace scenes {

using namespace common::resources;

class Join : public LuaBinding<bool( std::string )> {
public:
    bool operator()( const std::string& filename ) const {
        FUNC_LOGD( "scenes::join( filename=%s )", filename.c_str() );
        return m_ctx->getExecutor().busJoin( m_ctx->getModule(), "extensions.zwave", filename.c_str() );
    }
};

class Subscribe : public LuaBinding<void( std::string )> {
public:
    void operator()( std::string scriptName ) const {
        FUNC_LOGD( "scenes::subscribe( scriptName=%s )", scriptName.c_str() );
        m_ctx->getExecutor().send<bus::command::SetHandler>( SCENES_NAME, bus::BusAddrInvalid, scriptName );
    }
};

class Unsubscribe : public LuaBinding<void( std::string )> {
public:
    void operator()( std::string scriptName ) const {
        FUNC_LOGD( "scenes::unsubscribe( scriptName=%s )", scriptName.c_str() );
        m_ctx->getExecutor().send<bus::command::SetHandler>( SCENES_NAME, bus::BusAddrInvalid, scriptName );
    }
};

class ModifyScenes
: public Overloaded<LuaBinding<void( std::unique_ptr<::common::ArrayValue>, std::unique_ptr<::common::ArrayValue> )>,
                    LuaBinding<void( std::unique_ptr<::common::ArrayValue> )>> {
public:
    void operator()( std::unique_ptr<::common::ArrayValue> modify_commands,
                     std::unique_ptr<::common::ArrayValue> block_ids ) const {
        FUNC_LOGW( "scenes::modify_scenes" );
    }

    void operator()( std::unique_ptr<::common::ArrayValue> modify_commands ) const {
        FUNC_LOGW( "scenes::modify_scenes" );
    }
};

}  // namespace scenes
}  // namespace bindings
}  // namespace module
}  // namespace lua
