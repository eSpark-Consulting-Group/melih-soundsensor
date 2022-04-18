#pragma once

#include <cassert>
#include "Lua.h"
#include "LuaBindingExecutor.hpp"

namespace lua::module {

/**
 *  Provides interface to retrieve data from Lua stack.
 */
class LuaBindingContext {
public:
    explicit LuaBindingContext(::lua_State* lua )
    : m_lua( lua ) {}

    /**
     * Retrieves a pointer to IModule instance stored in Lua stack.
     * @return Pointer to user data reinterpret_cast'ed to IModule*.
     */
    ::lua::IModule& getModule() const {
        auto module = reinterpret_cast<::lua::IModule*>(::lua_touserdata( m_lua, lua_upvalueindex( 1 ) ) );
        assert( module );
        return *module;
    }

    ::lua::Executor& getExecutor() const {
        return getModule().getExecutor();
    }

    std::string getPluginId() const {
        return ::lua::IModule::getPluginId( m_lua );
    }

    ::gateways::IGatewayUserDataStorage& getGatewayUserDataStorage() const {
        return getModule().getExecutor().getGatewayUserDataStorage();
    }

    void setError( const char* errorString ) {
        m_errorString = errorString;
    }

    const char* getError() const {
        return m_errorString;
    }

private:
    ::lua_State* m_lua = nullptr;
    const char* m_errorString = nullptr;
};

}  // namespace lua::module
