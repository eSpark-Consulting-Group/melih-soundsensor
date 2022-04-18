#include "LuaModuleScenes.h"
#include "bindings/LuaScenesBindings.hpp"
#include "bindings/LuaBindingHelper.h"

namespace lua {
namespace module {

bool luaScenes::initialize() {
    return true;
}

luaScenes::luaScenes( Executor& exec, const char* name )
: IModule( exec, name ) {}

int luaScenes::regModule( lua_State* lua ) {
    static const auto bindArray =
        lua::createBindingsMap( luaL_Reg{ "join", bindings::luaBinding<bindings::scenes::Join> },
                                luaL_Reg{ "subscribe", bindings::luaBinding<bindings::scenes::Subscribe> },
                                luaL_Reg{ "unsubscribe", bindings::luaBinding<bindings::scenes::Unsubscribe> },
                                luaL_Reg{ "modify_scenes", bindings::luaBinding<bindings::scenes::ModifyScenes> } );

    return lua::createModuleByBindingsMap( lua, bindArray, this );
}

}  // namespace module

}  // namespace lua
