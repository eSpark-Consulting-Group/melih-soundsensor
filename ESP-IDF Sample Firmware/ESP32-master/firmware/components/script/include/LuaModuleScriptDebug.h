#pragma once

#include "LuaModule.h"

#include <string>
#include <vector>
#include <memory>

#include <StorageArFile.h>
#include <StorageZipFile.h>

namespace lua {

namespace module {

class ScriptDebug final : public IModule {
public:
    ScriptDebug( Executor& exec, const char* name );
    virtual ~ScriptDebug() = default;

    bool initialize() override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::PluginRun& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::CloudItemValueSet& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::CloudDictionaryValue& command ) override;
    void visit( const bus::BusAddr from, const bus::BusAddr to, bus::command::CloudForceRemove& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::commands::DeviceSettingValueSet& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::commands::DeviceSettingDictionaryValueSet& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::commands::GatewaySettingValueSet& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                settings::commands::DeviceSettingReset& command ) override;
    void visit( const bus::BusAddr from,
                const bus::BusAddr to,
                bus::command::HubDeviceFirmwareUpdate& command ) override;

private:
    bool readScriptFromArchive( std::string script_name );
    bool loadFileToLua();

public:
    static constexpr int BUFFER_SIZE = 256;
    static const char* codeReader( lua_State* lua, void* ud, size_t* osize );

private:
    static constexpr char CHUNK_NAME[] = "code";
};

}  // namespace module

}  // namespace lua
