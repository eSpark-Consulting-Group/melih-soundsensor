#pragma once
#include <list>
#include <string>
#include <cstdint>

#include <plugin_info.h>
#include <plugin_config.h>
#include <plugin_repository.h>
#include <plugin_graph.h>
#include <StorageFileFormat.h>
#include "Bus.h"

namespace common {
class JsonStreamGenerator;
};  // namespace common

namespace plugin {

class PluginManager : public bus::IBusClient {
public:
    enum class Result {
        OK = 0,

        WrongArguments,
        Busy,
        NotFound,
        NotSupported,
        SystemError
    };

    using Revision = std::uint32_t;

    class RevisionChecker {
    public:
        bool operator()();

    private:
        Revision m_revision = 0;
    };

    static constexpr char DIRECTORY_NAME[] = "plugin";
    static constexpr std::uint8_t MAX_FILENAME_LENGTH = 64;
    static constexpr std::uint16_t ASYNC_TASK_STACK_SIZE = 6 * 1024;
    static constexpr std::uint8_t ASYNC_TASK_PRIORITY = 10;
    static constexpr char CONFIG_FILENAME[] = "config.json";

    static PluginManager& get();

    // Revision is internal runtime number of plugins list changing.
    // It will be better to implement plaguns changes handling in following way:
    //    - ensure actual state of plugin list each time after update
    //    - make plugin manager able to send bus events
    //    - send "plugins_changed" or "plugins_updated" event on each change
    Revision getRevision() const;

    const std::list<PluginURI>& getInstalledPluginList() const;
    bool updateInstalledPluginList();
    /**
     * Removes all unsupported files stored in archive root directory.
     * The file is unsupported if it's format is not equal to supportedFormat parameter.
     * @param pluginId          The ID of a plugin to be matched to archive files (e.g. "zwave", "scene_blocks", etc.)
     * @param supportedFormat   Archive format that the system currently supports
     */
    void removeLegacyArchives( const std::string& pluginId, storage::FileFormat supportedFormat );
    bool installPlugin( PluginRepository& repository, const PluginInfo& plugin );
    bool installPlugin( const PluginURI& pluginURI, const std::string& filename, bool inflate = false );
    bool installDefaultPlugins( bool reinstall = true );
    bool asyncInstallPlugins( std::list<PluginURI>&& pluginURI );
    bool uninstallPlugin( const PluginURI& plugin );
    bool uninstallPluginWithDependencies( const PluginInfo& plugin );
    bool asyncUninstallPluginWithDependencies( const PluginInfo& plugin );
    PluginURI getPluginURI( const PluginInfo& plugin ) const;
    PluginConfig getPluginConfig( const std::string& name );
    std::list<PluginInfo> getPluginsRequiredFor( const PluginInfo& plugin ) const;
    std::list<PluginInfo> getPluginsDependantOn( const PluginInfo& plugin ) const;
    static bool checkPluginFile( const std::string& filename );
    bool checkPluginDependencies( const PluginInfo& plugin ) const;
    bool checkCycleDependency( const PluginInfo& plugin ) const;

    /// INFO: API -> http://log.ezlo.lan/new/hub/hub_platform/extensions_manager/
    Result getInstalled( common::JsonStreamGenerator& result );
    Result getAll( common::JsonStreamGenerator& result );
    Result getDependantOn( common::JsonStreamGenerator& result, const char* pluginId, const char* pluginVer );
    Result doInstall( common::JsonStreamGenerator& result, std::list<plugin::PluginURI>&& pluginURI );
    Result doUninstall( common::JsonStreamGenerator& result, const char* pluginId, const char* pluginVer );

private:
    static void pluginInstallationTask( void* pvParameters );
    static std::string getPluginFullFilenamePrefix();
    static std::string createPluginFilename( const PluginInfo& plugin, const storage::FileFormat file_format );
    static PluginInfo getPluginInfo( const std::string& filename, const storage::FileFormat file_format );

public:
    static PluginConfig getPluginConfig( const std::string& filename, const storage::FileFormat file_format );

private:
    static void pluginUninstallWithDependenciesTask( void* param );
    std::list<PluginURI> getUninstallDependencies( const PluginInfo& plugin ) const;
    static PluginID getHashedID( const PluginID& id );
    PluginGraph getInstalledPluginGraph() const;
    static bool verifyFileChecksum( const std::string& filename );
    static bool inflateToFile( const char* filename, const char* ptr, std::size_t size );

    PluginManager();

    PluginManager( const PluginManager& ) = delete;
    PluginManager& operator=( const PluginManager& ) = delete;

private:
    std::list<PluginURI> m_pluginsList;
    bool m_isPluginsListDirty = true;
    Revision m_revision = 0;
};

}  // namespace plugin
