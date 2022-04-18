#pragma once

#include <memory>
#include <map>
#include <lua.hpp>

#include "LuaModule.h"
#include "BinaryFileStorageFactory.h"
#include "KeyValueStorage.h"

#include "plugin_info.h"

namespace lua {

namespace module {

class Storage final : public IModule {
public:
    Storage( Executor& exec, const char* name, storage::BinaryFileStorageFactory& storageFactory );
    ~Storage() = default;

    bool initialize() override;
    int regModule( lua_State* lua ) override;
    void releaseStorage();

    // LUA API
    static int lua_get_bool( lua_State* lua );
    static int lua_get_number( lua_State* lua );
    static int lua_get_string( lua_State* lua );
    static int lua_get_table( lua_State* lua );
    static int lua_set_bool( lua_State* lua );
    static int lua_set_number( lua_State* lua );
    static int lua_set_string( lua_State* lua );
    static int lua_set_table( lua_State* lua );
    static int lua_exists( lua_State* lua );
    static int lua_delete( lua_State* lua );
    static int lua_delete_all( lua_State* lua );

private:
    // Overloads for lua_Number types
    static bool storeNumber( Storage& self, const char* key, float& value );
    static bool storeNumber( Storage& self, const char* key, double& value );
    static bool loadNumber( Storage& self, const char* key, float& value );
    static bool loadNumber( Storage& self, const char* key, double& value );

    storage::KeyValueStorage& getCurrentStorage();

private:
    storage::BinaryFileStorageFactory& m_storageFactory;
    std::map<plugin::PluginID, storage::KeyValueStorageFactory::StoragePtr> m_storages;
};

}  // namespace module

}  // namespace lua
