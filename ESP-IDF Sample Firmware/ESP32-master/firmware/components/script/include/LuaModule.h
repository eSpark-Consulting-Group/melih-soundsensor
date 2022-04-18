#pragma once

#include <string>
#include <vector>
#include <memory>

#include <lua.hpp>

#include <BusCommand.h>
#include <IValue.h>
#include <gason.hpp>

namespace lua {

extern const char luaDataKey[];
extern const char luaRunningPluginIdKey[];

class Executor;

class IModule : public bus::IBusCommandVisitor {
public:
    virtual ~IModule() = default;

    virtual bool initialize() = 0;

    virtual int regModule( lua_State* lua ) {
        return 0;
    }

    static std::string getPluginId( lua_State* luaState );
    static void setPluginId( lua_State* luaState, const char* pluginId );
    static void printLuaErrorDump( lua_State* lua );
    static common::IValue::UPtr createValueFromStack( lua_State* lua );
    static common::IValue::UPtr createValueFromStack( lua_State* lua, int idx );
    Executor& getExecutor() {
        return m_exec;
    }

protected:
    IModule( Executor& exec, const char* name );
    const char* getName() {
        return m_name;
    }

    static bool isLuaArray( lua_State*, int idx );

    static int getFieldAsInteger( lua_State* lua, const char* key );
    static std::string getStringField( lua_State* lua, const char* key );
    static bool getBooleanField( lua_State* lua, const char* key, bool defaultValue = false );
    static int getIntegerField( lua_State* lua, const char* key );
    static float getNumberField( lua_State* lua, const char* key );
    static std::vector<std::string> getArrayOfStringField( lua_State* lua, const char* key );
    static int popInteger( lua_State* lua );
    static float popNumber( lua_State* lua );

    static bool jSonToLuaStack( lua_State* lua, gason::JsonValue& obj );
    static common::IValue::UPtr createValueFromField( lua_State* lua, const char* key );
    static void stackDump( lua_State* lua );
    Executor& m_exec;

private:
    const char* m_name;
};

}  // namespace lua
