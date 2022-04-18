#include <plugin_manager.h>
#include <addon_manager.h>

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <sys/types.h>
#include <dirent.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <miniz.h>

#include <DebugUtils.h>
#include <plugin_downloader.h>
#include <Volume.h>
#include <Storage.h>
#include <StorageFile.h>
#include <StorageTarFile.h>
#include <StorageZipFile.h>
#include <StorageMolfarFile.h>
#include <DebugUtils.h>
#include <plugin_graph.h>
#include <Utils.h>
#include <StorageArchiveFileFactory.h>
#include <JsonStreamGenerator.h>
#include <CommonResources.h>
#include <scriptholder.h>
#include "scenes/Utils.h"
#include <soc/soc.h>
#include "nma/responces/SoftwareUpdateProgress.h"
#include "Thread.h"
#include "fstream"

namespace plugin {

constexpr char PluginManager::DIRECTORY_NAME[];
constexpr char PluginManager::CONFIG_FILENAME[];

using DIR_ptr_t = std::unique_ptr<DIR, void ( * )( DIR* )>;

static void dir_deleter( DIR* handle ) {
    if( handle ) {
        closedir( handle );
    }
}

bool PluginManager::RevisionChecker::operator()() {
    const auto prevRevision = m_revision;
    PluginManager::get().updateInstalledPluginList();
    m_revision = PluginManager::get().getRevision();
    return m_revision == prevRevision;
}

PluginManager::PluginManager() {
    auto dir_name = getPluginFullFilenamePrefix();
    DIR* dir = opendir( dir_name.c_str() );
    if( dir ) {
        auto res = closedir( dir );
        assert( 0 == res );
    }
    else {
        assert( ENOENT == errno );
        auto res = mkdir( dir_name.c_str(), 0 );
        assert( 0 == res );
    }

    if( !bus::Bus::get().join( "plugin_manager", *this ) ) {
        FUNC_LOGW( "Can't join to bus" );
    }
}

PluginManager& PluginManager::get() {
    static PluginManager instance;
    return instance;
}

PluginManager::Revision PluginManager::getRevision() const {
    return m_revision;
}

const std::list<PluginURI>& PluginManager::getInstalledPluginList() const {
    return m_pluginsList;
}

bool PluginManager::updateInstalledPluginList() {
    if( !m_isPluginsListDirty )
        return true;

    auto plug_path = getPluginFullFilenamePrefix();
    DIR_ptr_t dir( opendir( plug_path.c_str() ), &dir_deleter );
    if( !dir )
        return false;

    seekdir( dir.get(), 2 );  // skip current "." and upper ".." directory entries

    std::list<PluginURI> pluginsList;
    while( const auto entry = readdir( dir.get() ) ) {
        if( entry->d_type != DT_REG ) {
            FUNC_LOGW( "Unexpected plugin directory entry %s with type %i", entry->d_name, entry->d_type );
            continue;
        }
        std::string filename( entry->d_name );
        PluginURI plugin_uri;
        plugin_uri.uri = plug_path + "/" + filename;
        plugin_uri.format = storage::ArchiveFileFactory::getFileFormatFromName( plugin_uri.uri.c_str() );
        plugin_uri.plugin = getPluginInfo( plugin_uri.uri, plugin_uri.format );
        if( !plugin_uri.plugin.id.empty() ) {
            pluginsList.push_back( std::move( plugin_uri ) );
        }
    }
    m_pluginsList = std::move( pluginsList );
    m_isPluginsListDirty = false;
    ++m_revision;
    return true;
}

void PluginManager::removeLegacyArchives( const std::string& pluginId, storage::FileFormat supportedFormat ) {
    auto pathPrefix = getPluginFullFilenamePrefix();
    if( auto dir = DIR_ptr_t(::opendir( pathPrefix.c_str() ), &dir_deleter ) ) {
        // seek over "." and ".."
        ::seekdir( dir.get(), 2 );
        while( const auto dirEntry = readdir( dir.get() ) ) {
            if( dirEntry->d_type == DT_REG ) {
                if(::common::stringStartsWith( dirEntry->d_name, getHashedID( pluginId ).c_str() ) ) {
                    auto format = storage::ArchiveFileFactory::getFileFormatFromName( dirEntry->d_name );

                    if( ( supportedFormat != storage::FileFormat::UNKNOWN && format != supportedFormat ) ||
                        ( supportedFormat == storage::FileFormat::UNKNOWN &&
                          format == storage::FileFormat::UNKNOWN ) ) {
                        storage::Storage::get().remove( pathPrefix + "/" + dirEntry->d_name );
                        FUNC_LOGI( "Legacy archive is removed: %s", dirEntry->d_name );
                    }
                }
            }
        }
    }
}

bool PluginManager::installPlugin( PluginRepository& repository, const PluginInfo& plugin ) {
    PluginURI pluginURI = repository.getPluginURI( plugin.id, plugin.version );
    return installPlugin( pluginURI, createPluginFilename( plugin, pluginURI.format ) );
}

bool PluginManager::installPlugin( const PluginURI& pluginURI, const std::string& filename, bool inflate ) {
    bool result{ true };
    PluginDownloader downloader;
    if( !downloader.download( pluginURI.uri.c_str(), filename.c_str() ) ) {
        result = false;
    }
    if( inflate ) {
        storage::Storage& file_storage = storage::Storage::get();
        std::ifstream input( filename.c_str(), std::ios::binary );
        std::vector<unsigned char> buffer( std::istreambuf_iterator<char>( input ), {} );
        input.close();
        file_storage.remove( filename.c_str() );
        if( !inflateToFile( filename.c_str(), (const char*)buffer.data(), buffer.size() ) ) {
            FUNC_LOGE( "Failed inflating plugin to file" );
            return false;
        }
    }
#ifdef TAR_FORMAT
    if( storage::ArchiveFileFactory::getFileFormatFromName( filename.c_str() ) == storage::FileFormat::TAR ) {
        if( !storage::TarFile::createIndex( filename.c_str() ) ) {
            FUNC_LOGE( "Failed to create idx for %s", filename.c_str() );
        }
    }
#endif
#ifdef MOLFAR_FORMAT
    if( storage::ArchiveFileFactory::getFileFormatFromName( filename.c_str() ) == storage::FileFormat::MOLFAR ) {
        if( !storage::MolfarFile::createIndex( filename.c_str() ) ) {
            FUNC_LOGE( "Failed to create idx for %s", filename.c_str() );
        }
    }
#endif
    if( result )
        m_isPluginsListDirty = true;

    return result;
}

bool PluginManager::installDefaultPlugins( bool reinstall ) {
    for( std::size_t i = 0;; ++i ) {
        auto pluginDescription = getScritpDesc( i );
        if( !pluginDescription ) {
            break;
        }
        PluginInfo pluginInfo;
        pluginInfo.id = pluginDescription->name;
        pluginInfo.version.major = pluginDescription->major;
        pluginInfo.version.minor = pluginDescription->minor;

        removeLegacyArchives( pluginInfo.id, storage::FileFormat::UNKNOWN );
    }

    updateInstalledPluginList();
    auto installed_plugins = getInstalledPluginList();

    m_isPluginsListDirty = true;
    bool updateScenes = false;
    for( std::size_t i = 0;; ++i ) {
        auto pluginDescription = getScritpDesc( i );
        if( !pluginDescription ) {
            break;
        }
        PluginInfo pluginInfo;
        pluginInfo.id = pluginDescription->name;
        pluginInfo.version.major = pluginDescription->major;
        pluginInfo.version.minor = pluginDescription->minor;
        auto format = storage::ArchiveFileFactory::getFileFormatFromName( pluginDescription->extension );
        if( format == storage::FileFormat::UNKNOWN ) {
            FUNC_LOGE(
                "Unsupported file format of %s plugin: %s", pluginDescription->name, pluginDescription->extension );
            continue;
        }
        bool inflate = false;
#ifdef TAR_FORMAT
        if( format == storage::FileFormat::TAR_ZZ ) {
            inflate = true;
            format = storage::FileFormat::TAR;
        }
#endif
#ifdef MOLFAR_FORMAT
        if( format == storage::FileFormat::MOLFAR_ZZ ) {
            inflate = true;
            format = storage::FileFormat::MOLFAR;
        }
#endif
        auto plugin_filename = createPluginFilename( pluginInfo, format );

        // Check if plugin with the same ID already exists
        auto installed_plugin_it = std::find_if(
            installed_plugins.begin(), installed_plugins.end(), [&]( const PluginURI& plugin_uri ) -> bool {
                return plugin_uri.plugin.id == pluginInfo.id;
            } );

        bool is_plugin_exists = installed_plugin_it != installed_plugins.end();
        bool install_default_plugin = !is_plugin_exists;

        if( reinstall ) {
            if( is_plugin_exists ) {
                FUNC_LOGI( "Uninstall plugin ID: %s", installed_plugin_it->plugin.id.c_str() );
                if( !uninstallPlugin( *installed_plugin_it ) ) {
                    FUNC_LOGE( "Failed uninstalling plugin: %s", installed_plugin_it->plugin.id.c_str() );
                }
            }
            install_default_plugin = true;
        }

        if( install_default_plugin ) {
            FUNC_LOGI( "Install default plugin. ID: %s, filename: %s", pluginInfo.id.c_str(), plugin_filename.c_str() );
            removeLegacyArchives( pluginInfo.id, format );
            if( inflate ) {
                if( !inflateToFile( plugin_filename.c_str(),
                                    (const char*)pluginDescription->begin,
                                    pluginDescription->end - pluginDescription->begin ) ) {
                    FUNC_LOGE( "Failed inflating plugin to file" );
                    return false;
                }
            }
            else {
                storage::File plugin;
                if( !plugin.open( plugin_filename.c_str(), "w", std::chrono::milliseconds( 100 ), false ) ) {
                    FUNC_LOGE( "Failed opening plugin file" );
                    return false;
                }
                if( !plugin.write( pluginDescription->begin, pluginDescription->end - pluginDescription->begin ) ) {
                    FUNC_LOGE( "Failed writing to plugin file" );
                    return false;
                }
            }
#ifdef TAR_FORMAT
            if( format == storage::FileFormat::TAR ) {
                if( !storage::TarFile::createIndex( plugin_filename.c_str() ) ) {
                    FUNC_LOGE( "Failed to create idx for %s", plugin_filename.c_str() );
                    return false;
                }
            }
#endif
#ifdef MOLFAR_FORMAT
            if( format == storage::FileFormat::MOLFAR ) {
                if( !storage::MolfarFile::createIndex( plugin_filename.c_str() ) ) {
                    FUNC_LOGE( "Failed to create idx for %s", plugin_filename.c_str() );
                    return false;
                }
            }
#endif
            if( pluginInfo.id == common::resources::tokens::SceneBlocks ) {
                updateScenes = true;
            }
        }
    }

    updateInstalledPluginList();

    if( updateScenes ) {
        constexpr bool isRebuild = true;
        if( scenes::updateHubScenesTriggerDevicesList( isRebuild ) ) {
            FUNC_LOGW( "Scenes templates was updated successfully" );
        }
        else {
            FUNC_LOGE( "Update scenes templates was failed!" );
        }
    }

    auto installed_plugins_res = getInstalledPluginList();
    return true;
}

namespace {
struct InstallationTaskParameter {
    PluginManager& pluginsManager;
    std::list<PluginURI> pluginURI;
};

struct UninstallationTaskParameter {
    PluginManager& pluginsManager;
    PluginInfo pluginInfo;
};
}  // namespace

void PluginManager::pluginInstallationTask( void* pvParameters ) {
    {
        std::unique_ptr<InstallationTaskParameter> param{ reinterpret_cast<InstallationTaskParameter*>(
            pvParameters ) };
        uint8_t step = 0u;
        const uint8_t plugin_count = param->pluginURI.size();
        auto getCompletion = [&plugin_count]( int step ) -> int { return step * 100 / ( plugin_count ); };
        PluginManager& plugin_manager = param->pluginsManager;
        for( const auto& pluginURI : param->pluginURI ) {
            auto cmd = nma::responces::HubSoftwareUpdateProgress::createPluginInstall(
                pluginURI.plugin.id,
                pluginURI.plugin.version.toString(),
                nma::responces::HubSoftwareUpdateProgress::Status::Started,
                getCompletion( step ) );
            plugin_manager.send( "nma", std::move( cmd ) );

            ++step;

            std::string plugin_filename = createPluginFilename( pluginURI.plugin, pluginURI.format );

            bool inflate = false;
            storage::FileFormat format = pluginURI.format;
#ifdef TAR_FORMAT
            if( pluginURI.format == storage::FileFormat::TAR_ZZ ) {
                inflate = true;
                plugin_filename = createPluginFilename( pluginURI.plugin, storage::FileFormat::TAR );
                format = storage::FileFormat::TAR;
            }
#endif
#ifdef MOLFAR_FORMAT
            if( pluginURI.format == storage::FileFormat::MOLFAR_ZZ ) {
                inflate = true;
                plugin_filename = createPluginFilename( pluginURI.plugin, storage::FileFormat::MOLFAR );
                format = storage::FileFormat::MOLFAR;
            }
#endif
            bool check_plugin = plugin_manager.installPlugin( pluginURI, plugin_filename, inflate );

            check_plugin = check_plugin && checkPluginFile( plugin_filename );
            check_plugin = check_plugin && plugin_manager.updateInstalledPluginList();
            check_plugin =
                check_plugin && plugin_manager.checkCycleDependency( getPluginInfo( plugin_filename, format ) );
            if( check_plugin ) {
                auto cmd = nma::responces::HubSoftwareUpdateProgress::createPluginInstall(
                    pluginURI.plugin.id,
                    pluginURI.plugin.version.toString(),
                    nma::responces::HubSoftwareUpdateProgress::Status::Finished,
                    getCompletion( step ) );
                plugin_manager.send( "nma", std::move( cmd ) );

                const auto& plugId = pluginURI.plugin.id;
                if( plugId == common::resources::tokens::SceneBlocks ) {
                    constexpr bool isRebuild = true;
                    if( scenes::updateHubScenesTriggerDevicesList( isRebuild ) ) {
                        FUNC_LOGW( "Scenes templates was updated successfully" );
                    }
                    else {
                        FUNC_LOGE( "Update scenes templates was failed!" );
                    }
                }
                FUNC_LOGI( "Success installing [%s] plugin from [%s]", plugId.c_str(), pluginURI.uri.c_str() );
            }
            else {
                storage::Storage& file_storage = storage::Storage::get();
                if( file_storage.isExist( plugin_filename.c_str() ) ) {
#ifdef TAR_FORMAT
                    if( format == storage::FileFormat::TAR ) {
                        if( !storage::TarFile::createIndex( plugin_filename.c_str() ) ) {
                            FUNC_LOGE( "Failed to create idx for %s", plugin_filename.c_str() );
                        }
                        // storage::TarFile::clearCache();
                    }
#endif
#ifdef MOLFAR_FORMAT
                    if( format == storage::FileFormat::MOLFAR ) {
                        if( !storage::MolfarFile::createIndex( plugin_filename.c_str() ) ) {
                            FUNC_LOGE( "Failed to create idx for %s", plugin_filename.c_str() );
                        }
                    }
#endif
                    file_storage.remove( plugin_filename.c_str() );
                }
                bool isFailed = true;
                auto cmd = nma::responces::HubSoftwareUpdateProgress::createPluginInstall(
                    pluginURI.plugin.id,
                    pluginURI.plugin.version.toString(),
                    nma::responces::HubSoftwareUpdateProgress::Status::Finished,
                    getCompletion( step ),
                    isFailed );
                plugin_manager.send( "nma", std::move( cmd ) );
                FUNC_LOGE(
                    "Fail installing [%s] plugin from [%s]", pluginURI.plugin.id.c_str(), pluginURI.uri.c_str() );
            }
        }
    }
    vTaskDelete( NULL );
}

bool PluginManager::asyncInstallPlugins( std::list<PluginURI>&& pluginURI ) {
    // TODO: verify free space

    TaskHandle_t handleOfTask;

    InstallationTaskParameter* param = new InstallationTaskParameter{ *this, std::move( pluginURI ) };
    BaseType_t res = xTaskCreate( &PluginManager::pluginInstallationTask,
                                  "PLUGIN_INSTALL",
                                  ASYNC_TASK_STACK_SIZE,
                                  reinterpret_cast<void*>( param ),
                                  ASYNC_TASK_PRIORITY,
                                  &handleOfTask );
    if( pdPASS != res ) {
        FUNC_LOGE( "xTaskCreate return %d", res );
        delete param;
        return false;
    }
    return true;
}

bool PluginManager::uninstallPlugin( const PluginURI& plugin ) {
    bool result{ true };
    if( !storage::Storage::get().remove( plugin.uri.c_str() ) ) {
        FUNC_LOGE( "Fail removing plugin file: %s", plugin.uri.c_str() );
        result = false;
    }
    else {
        m_isPluginsListDirty = true;
    }

    return result;
}

bool PluginManager::uninstallPluginWithDependencies( const PluginInfo& plugin ) {
    updateInstalledPluginList();
    auto plugin_uninstall_list = getUninstallDependencies( plugin );
    bool result{ true };
    uint8_t step = 0u;
    const uint8_t plugin_count = plugin_uninstall_list.size();
    auto getCompletion = [&plugin_count]( uint8_t step ) -> uint8_t { return step * 100u / ( plugin_count ); };
    for( const auto& next_plugin : plugin_uninstall_list ) {
        auto cmd = nma::responces::HubSoftwareUpdateProgress::createPluginUninstall(
            next_plugin.plugin.id,
            next_plugin.plugin.version.toString(),
            nma::responces::HubSoftwareUpdateProgress::Status::Started,
            getCompletion( step ) );
        send( "nma", std::move( cmd ) );

        ++step;
        result = uninstallPlugin( next_plugin );
        const Version& ver = next_plugin.plugin.version;
        if( result ) {
            auto cmd = nma::responces::HubSoftwareUpdateProgress::createPluginUninstall(
                next_plugin.plugin.id,
                next_plugin.plugin.version.toString(),
                nma::responces::HubSoftwareUpdateProgress::Status::Finished,
                getCompletion( step ) );
            send( "nma", std::move( cmd ) );

            FUNC_LOGI( "Success uninstall plugin %s %d.%d", next_plugin.plugin.id.c_str(), ver.major, ver.minor );
        }
        else {
            bool isFailed = true;
            auto cmd = nma::responces::HubSoftwareUpdateProgress::createPluginUninstall(
                next_plugin.plugin.id,
                next_plugin.plugin.version.toString(),
                nma::responces::HubSoftwareUpdateProgress::Status::Finished,
                getCompletion( step ),
                isFailed );
            send( "nma", std::move( cmd ) );
            FUNC_LOGE( "Fail uninstalling plugin %s %d.%d", next_plugin.plugin.id.c_str(), ver.major, ver.minor );
            break;
        }
    }
    return result;
}

bool PluginManager::asyncUninstallPluginWithDependencies( const PluginInfo& plugin ) {
    UninstallationTaskParameter* param = new UninstallationTaskParameter{ *this, plugin };
    TaskHandle_t handleOfTask;
    if( pdPASS != xTaskCreate( &PluginManager::pluginUninstallWithDependenciesTask,
                               "PLUGIN_UNINSTALL",
                               ASYNC_TASK_STACK_SIZE,
                               reinterpret_cast<void*>( param ),
                               ASYNC_TASK_PRIORITY,
                               &handleOfTask ) ) {
        delete param;
        return false;
    }
    return true;
}

PluginURI PluginManager::getPluginURI( const PluginInfo& plugin ) const {
    PluginURI plugin_uri;
    auto plugin_it =
        std::find_if( m_pluginsList.begin(), m_pluginsList.end(), [&plugin]( const auto& next_plugin_uri ) {
            return next_plugin_uri.plugin == plugin;
        } );
    if( plugin_it != m_pluginsList.end() ) {
        plugin_uri = *plugin_it;
    }
    return plugin_uri;
}

PluginConfig PluginManager::getPluginConfig( const std::string& name ) {
    if( updateInstalledPluginList() ) {
        const auto& list = getInstalledPluginList();
        for( const auto& i : list ) {
            if( name == i.plugin.id ) {
                return getPluginConfig( i.uri, i.format );
            }
        }
    }
    return PluginConfig();
}

std::list<PluginInfo> PluginManager::getPluginsRequiredFor( const PluginInfo& plugin ) const {
    PluginGraph graph = getInstalledPluginGraph();
    return graph.topologicalSort( plugin );
}

std::list<PluginInfo> PluginManager::getPluginsDependantOn( const PluginInfo& plugin ) const {
    PluginGraph graph = getInstalledPluginGraph();
    graph.reverseEdges();
    return graph.topologicalSort( plugin );
}

bool PluginManager::checkPluginFile( const std::string& filename ) {
    bool result = verifyFileChecksum( filename );
    // Do file contain plugin inforamtion
    result =
        result && !getPluginConfig( filename, storage::ArchiveFileFactory::getFileFormatFromName( filename.c_str() ) )
                       .getPlugin()
                       .id.empty();
    return result;
}

bool PluginManager::checkPluginDependencies( const PluginInfo& plugin ) const {
    PluginURI pluginURI = getPluginURI( plugin );
    // Check do we have this plugin
    if( pluginURI.uri.empty() ) {
        return false;
    }
    PluginConfig plugin_config = getPluginConfig( pluginURI.uri, pluginURI.format );
    // Check only first oreder dependencies
    for( const auto& next_plugin : plugin_config.getPluginDependency() ) {
        if( std::none_of(
                m_pluginsList.begin(), m_pluginsList.end(), [&next_plugin]( const auto& next_pluginURI ) -> bool {
                    return next_pluginURI.plugin == next_plugin;
                } ) ) {
            return false;
        }
    }
    return true;
}

bool PluginManager::checkCycleDependency( const PluginInfo& plugin ) const {
    PluginGraph graph = getInstalledPluginGraph();
    if( graph.topologicalSort( plugin ).empty() ) {
        return false;
    }
    return true;
}

std::string PluginManager::getPluginFullFilenamePrefix() {
    return std::string( storage::Volume::m_optPath ).append( "/" ).append( DIRECTORY_NAME );
}

std::string PluginManager::createPluginFilename( const PluginInfo& plugin, const storage::FileFormat file_format ) {
    char buf[MAX_FILENAME_LENGTH];
    const Version& ver = plugin.version;
    const char* extension = storage::ArchiveFileFactory::getFileExtension( file_format );
    int length = snprintf( &buf[0],
                           sizeof( buf ),
                           "%s/%s.%u.%u.%s",
                           getPluginFullFilenamePrefix().c_str(),
                           getHashedID( plugin.id ).c_str(),
                           ver.major,
                           ver.minor,
                           extension );
    if( length < 0 ) {
        return {};
    }
    return { buf };
}

PluginInfo PluginManager::getPluginInfo( const std::string& filename, const storage::FileFormat file_format ) {
    auto config = getPluginConfig( filename, file_format );
    if( config.getId().empty() ) {
        return {};
    }

    return config.getPlugin();
}

PluginConfig PluginManager::getPluginConfig( const std::string& filename, const storage::FileFormat file_format ) {
    std::unique_ptr<storage::ArchiveFile> plugin_file{ storage::ArchiveFileFactory::getArchiveFile( file_format ) };
    if( !plugin_file ) {
        FUNC_LOGE( "File %s has unknown format %i", filename.c_str(), (int)file_format );
        return {};
    }

    if( !plugin_file->open( filename.c_str(), CONFIG_FILENAME ) ) {
        FUNC_LOGE( "Can't open <%s> file in <%s> archive", CONFIG_FILENAME, filename.c_str() );
        return {};
    }

    size_t file_buffer_length = plugin_file->size();
    PluginConfig config;
    config.buff.resize( file_buffer_length );

    size_t read_bytes{ 0u };
    if( !plugin_file->read( config.buff.data(), config.buff.size(), &read_bytes ) ||
        read_bytes != file_buffer_length ) {
        FUNC_LOGE( "Fail reading <%s> file in <%s> archive", CONFIG_FILENAME, filename.c_str() );
        plugin_file->close();
        return {};
    }
    plugin_file->close();

    if( !config.jsonAlloc ||
        gason::JSON_PARSE_OK != gason::jsonParse( config.buff.data(), config.json, *( config.jsonAlloc ) ) ) {
        FUNC_LOGE( "Fail parsing <%s> file in <%s> archive", CONFIG_FILENAME, filename.c_str() );
        return {};
    }
    return config;
}

void PluginManager::pluginUninstallWithDependenciesTask( void* param ) {
    {
        std::unique_ptr<UninstallationTaskParameter> uninstallParameter(
            reinterpret_cast<UninstallationTaskParameter*>( param ) );

        // TODO: check return value and send appropriate notification
        uninstallParameter->pluginsManager.uninstallPluginWithDependencies( uninstallParameter->pluginInfo );
    }
    vTaskDelete( NULL );
}

std::list<PluginURI> PluginManager::getUninstallDependencies( const PluginInfo& plugin ) const {
    std::list<PluginURI> uninstall_plugin_list;
    for( const auto& next_plugin : getPluginsDependantOn( plugin ) ) {
        uninstall_plugin_list.push_back( getPluginURI( next_plugin ) );
    }
    if( uninstall_plugin_list.empty() ) {
        const Version& ver = plugin.version;
        FUNC_LOGE( "Can't find [%s %d.%d] plugin", plugin.id.c_str(), ver.major, ver.minor );
    }
    return uninstall_plugin_list;
}

PluginID PluginManager::getHashedID( const PluginID& id ) {
    std::hash<PluginID> hash_obj;
    const size_t hash = hash_obj( id );
    constexpr int name_length = sizeof( hash ) * 2 + 1;
    char name[name_length];
    name[name_length - 1] = 0;
    constexpr int mask = 0x0F;
    constexpr int shift = 0x61;
    for( int i = 0; i < sizeof( hash ); ++i ) {
        name[2 * i] = shift + ( hash >> ( 8 * i ) & mask );
        name[2 * i + 1] = shift + ( hash >> ( 8 * i + 4 ) & mask );
    }
    return { name };
}

PluginGraph PluginManager::getInstalledPluginGraph() const {
    PluginGraph graph;
    // Fill vertices
    for( const auto& next_plugin_uri : m_pluginsList ) {
        graph.addVertex( next_plugin_uri.plugin );
    }
    // Fill edges
    for( const auto& next_plugin_uri : m_pluginsList ) {
        PluginConfig plugin_config = getPluginConfig( next_plugin_uri.uri, next_plugin_uri.format );
        for( const auto& plugin : plugin_config.getPluginDependency() ) {
            if( !graph.addEdge( plugin_config.getPlugin(), plugin ) ) {
                FUNC_LOGE(
                    "Can't insert edge from: %s -> %s", plugin_config.getPlugin().id.c_str(), plugin.id.c_str() );
            }
        }
    }
    return graph;
}

bool PluginManager::verifyFileChecksum( const std::string& filename ) {
    // TODO: implement checksum verification
    return true;
}

PluginManager::Result PluginManager::getInstalled( common::JsonStreamGenerator& result ) {
    PluginManager::Result ret = PluginManager::Result::OK;
    bool ok = updateInstalledPluginList();
    const std::list<plugin::PluginURI>& plugins = getInstalledPluginList();
    ok = ok && result.writeOpeningObject();
    ok = ok && result.writePropertyKey( common::resources::tokens::Plugins );
    ok = ok && result.writeOpeningArray();
    constexpr size_t buffer_size = 16;
    char buffer[buffer_size];
    for( const auto& next_plugin : plugins ) {
        ok = ok && result.writeOpeningObject();
        ok = ok && result.writeProperty( common::resources::tokens::ExtensionID, next_plugin.plugin.id.c_str() );
        const plugin::Version& ver = next_plugin.plugin.version;
        ok = ok && ( 0 < snprintf( buffer, buffer_size, "%hhu.%hhu", ver.major, ver.minor ) );
        buffer[buffer_size - 1] = 0;
        ok = ok && result.writeProperty( common::resources::tokens::ExtensionVersion, buffer );
        ok = ok && result.writeClosingSymbol();
    }
    ok = ok && result.writeClosingSymbol();  // Plugin array
    ok = ok && result.writeClosingSymbol();  // Result object
    if( !ok ) {
        ret = PluginManager::Result::SystemError;
    }
    return ret;
}

PluginManager::Result PluginManager::getAll( common::JsonStreamGenerator& result ) {
    PluginManager::Result ret = PluginManager::Result::OK;
    constexpr size_t buffer_size = 16;
    char buffer[buffer_size];
    bool ok = true;
    std::list<addon::AddonInfo> addons = addon::AddonManager().getInstalledAddonList();
    ok = ok && result.writePropertyKey( common::resources::tokens::Addons );
    ok = ok && result.writeOpeningArray();
    for( const auto& next_addon : addons ) {
        ok = ok && result.writeOpeningObject();
        ok = ok && result.writeProperty( common::resources::tokens::ExtensionID, next_addon.id.c_str() );
        const plugin::Version& ver = next_addon.version;
        ok = ok && ( 0 < snprintf( buffer, buffer_size, "%hhu.%hhu", ver.major, ver.minor ) );
        buffer[buffer_size - 1] = 0;
        ok = ok && result.writeProperty( common::resources::tokens::ExtensionVersion, buffer );
        ok = ok && result.writeClosingSymbol();
    }
    ok = ok && result.writeClosingSymbol();  // Addons array
    ok = ok && updateInstalledPluginList();
    const std::list<plugin::PluginURI>& plugins = getInstalledPluginList();
    ok = ok && result.writePropertyKey( common::resources::tokens::Plugins );
    ok = ok && result.writeOpeningArray();
    for( const auto& next_plugin : plugins ) {
        ok = ok && result.writeOpeningObject();
        ok = ok && result.writeProperty( common::resources::tokens::ExtensionID, next_plugin.plugin.id.c_str() );
        const plugin::Version& ver = next_plugin.plugin.version;
        ok = ok && ( 0 < snprintf( buffer, buffer_size, "%hhu.%hhu", ver.major, ver.minor ) );
        buffer[buffer_size - 1] = 0;
        ok = ok && result.writeProperty( common::resources::tokens::ExtensionVersion, buffer );
        ok = ok && result.writeClosingSymbol();
    }
    ok = ok && result.writeClosingSymbol();  // Plugin array
    if( !ok ) {
        ret = PluginManager::Result::SystemError;
    }
    return ret;
}

PluginManager::Result PluginManager::getDependantOn( common::JsonStreamGenerator& result,
                                                     const char* pluginId,
                                                     const char* pluginVer ) {
    plugin::PluginInfo plugin;
    if( !pluginId || !strlen( pluginId ) || !pluginVer || !strlen( pluginVer ) ||
        ( sscanf( pluginVer, "%hhu.%hhu", &plugin.version.major, &plugin.version.minor ) != 2 ) ) {
        return PluginManager::Result::WrongArguments;
    }
    plugin.id = pluginId;

    PluginManager::Result ret = PluginManager::Result::OK;
    bool ok = updateInstalledPluginList();
    std::list<plugin::PluginInfo> plugins = getPluginsDependantOn( plugin );
    constexpr size_t buffer_size = 16;
    char buffer[buffer_size];
    ok = ok && result.writeOpeningObject();  // Result object
    ok = ok && result.writeProperty( common::resources::tokens::ExtensionID, plugin.id.c_str() );
    ok = ok && ( 0 < snprintf( buffer, buffer_size, "%hhu.%hhu", plugin.version.major, plugin.version.minor ) );
    ok = ok && result.writeProperty( common::resources::tokens::ExtensionVersion, buffer );
    ok = ok && result.writePropertyKey( "plugins" );
    ok = ok && result.writeOpeningArray();  // Plugin array
    for( const auto& next_plugin : plugins ) {
        if( next_plugin == plugin ) {
            break;  /// CHECK: maybe continue ??? - break: from original code
        }
        ok = ok && result.writeOpeningObject();
        ok = ok && result.writeProperty( common::resources::tokens::ExtensionID, next_plugin.id.c_str() );
        ok = ok &&
             ( 0 < snprintf( buffer, buffer_size, "%hhu.%hhu", next_plugin.version.major, next_plugin.version.minor ) );
        ok = ok && result.writeProperty( common::resources::tokens::ExtensionVersion, buffer );
        ok = ok && result.writeClosingSymbol();
    }
    ok = ok && result.writeClosingSymbol();  // Plugin array
    ok = ok && result.writeClosingSymbol();  // Result object
    if( !ok ) {
        ret = PluginManager::Result::SystemError;
    }
    return ret;
}

PluginManager::Result PluginManager::doInstall( common::JsonStreamGenerator& result,
                                                std::list<plugin::PluginURI>&& pluginURI ) {
    if( pluginURI.empty() ) {
        return PluginManager::Result::WrongArguments;
    }

    PluginManager::Result ret = PluginManager::Result::OK;
    if( !plugin::PluginManager::asyncInstallPlugins( std::move( pluginURI ) ) ) {
        ret = PluginManager::Result::SystemError;
    }

    return ret;
}

PluginManager::Result PluginManager::doUninstall( common::JsonStreamGenerator& result,
                                                  const char* pluginId,
                                                  const char* pluginVer ) {
    plugin::PluginInfo plugin;
    if( !pluginId || !strlen( pluginId ) || !pluginVer || !strlen( pluginVer ) ||
        ( sscanf( pluginVer, "%hhu.%hhu", &plugin.version.major, &plugin.version.minor ) != 2 ) ) {
        return PluginManager::Result::WrongArguments;
    }
    plugin.id = pluginId;

    PluginManager::Result ret = PluginManager::Result::OK;
    if( !asyncUninstallPluginWithDependencies( plugin ) ) {
        ret = PluginManager::Result::SystemError;
    }

    return ret;
}

bool PluginManager::inflateToFile( const char* filename, const char* ptr, std::size_t size ) {
    FUNC_LOGD( "Start inflate: %s", filename );
    std::size_t BUFF_SIZE = 8192;
    std::vector<unsigned char> buff( BUFF_SIZE );
    auto s_outbuf = buff.data();
    std::size_t n_decompressed = 0;
    std::size_t file_size = 0;
    mz_stream stream;
    memset( &stream, 0, sizeof( stream ) );
    stream.next_in = (const unsigned char*)ptr;
    stream.avail_in = size;
    stream.next_out = s_outbuf;
    stream.avail_out = BUFF_SIZE;
    storage::File plugin;
    bool& plugin_transaction = plugin.linkToTransactionStatus();

    if( mz_inflateInit2( &stream, MZ_DEFAULT_WINDOW_BITS ) ) {
        FUNC_LOGE( "Inflate init error" );
        goto error;
    }

    if( !plugin.open( filename, "w", std::chrono::milliseconds( 100 ), false ) ) {
        FUNC_LOGE( "Failed opening plugin file" );
        goto inflate_init_error;
    }

    for( ;; ) {
        int status;
        if( !stream.avail_in ) {
            stream.avail_in = 0;
        }

        status = mz_inflate( &stream, MZ_SYNC_FLUSH );

        if( ( status == MZ_STREAM_END ) || ( !stream.avail_out ) ) {
            // Output buffer is full, or decompression is done, so write buffer to output file.
            n_decompressed = BUFF_SIZE - stream.avail_out;
            file_size += n_decompressed;
            if( !plugin.write( s_outbuf, n_decompressed ) ) {
                FUNC_LOGE( "Failed to write decompressed data.  Error : %s.  BUFF_SIZE = %d, stream.avail_out = %d, "
                           "n_decompressed = %d ",
                           strerror( errno ),
                           BUFF_SIZE,
                           stream.avail_out,
                           n_decompressed );
                goto file_opened_error;
            }

            stream.next_out = s_outbuf;
            stream.avail_out = BUFF_SIZE;
        }

        if( status == MZ_STREAM_END )
            break;
        else if( status != MZ_OK ) {
            FUNC_LOGE( "inflate() failed with status %i!", status );
            goto file_opened_error;
        }

        core::this_thread::yield();
    }

    plugin.close();
    FUNC_LOGI( "Inflate successfully finished. Decompressed file size: %d", file_size );

    if( mz_inflateEnd( &stream ) != MZ_OK ) {
        FUNC_LOGE( "Fail to deinit inflation stream" );
        // TODO: It could be reasonable to delete plugin file
        goto error;
    }

    return true;

file_opened_error:
    plugin_transaction = false;
    plugin.close();
inflate_init_error:
    mz_inflateEnd( &stream );
error:
    return false;
}

}  // namespace plugin
