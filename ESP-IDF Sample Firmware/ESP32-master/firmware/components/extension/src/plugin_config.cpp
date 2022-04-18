
#include <plugin_config.h>
#include <CommonResources.h>
#include <DebugUtils.h>

namespace plugin {

using namespace common::resources;

PluginConfig::PluginConfig() {
    jsonAlloc = std::make_unique<gason::JsonAllocator>();
}

PluginConfig::PluginConfig( PluginConfig&& other ) {
    jsonAlloc = std::move( other.jsonAlloc );
    buff = std::move( other.buff );
    json = other.json;
}

PluginConfig& PluginConfig::operator=( PluginConfig&& other ) {
    jsonAlloc = std::move( other.jsonAlloc );
    buff = std::move( other.buff );
    json = other.json;
    return *this;
}

std::string PluginConfig::getId() {
    if( json( tokens::Id ) ) {
        return json( tokens::Id ).toString();
    }
    return {};
}

std::string PluginConfig::getType() {
    if( json( tokens::Type ) ) {
        return json( tokens::Type ).toString();
    }
    return {};
}

PluginInfo PluginConfig::getPlugin() {
    PluginInfo plugin;

    if( !jsonAlloc ) {
        FUNC_LOGE( "PluginConfig's json  is wrong" );
        return {};
    }

    if( !json( tokens::Id ) || !json( tokens::Version ) ) {
        FUNC_LOGE( "Fail Id " );
        return {};
    }

    if( json( tokens::Id ).toString() == nullptr || json( tokens::Version ).toString() == nullptr ) {
        FUNC_LOGE( "Fail Id value" );
        return {};
    }

    plugin.id.assign( json( tokens::Id ).toString() );

    if( 2 != sscanf( json( tokens::Version ).toString(), "%hhu.%hhu", &plugin.version.major, &plugin.version.minor ) ) {
        FUNC_LOGE( "Fail parsing  version" );
    }

    return plugin;
}

std::list<PluginInfo> PluginConfig::getPluginDependency() {
    std::list<PluginInfo> plugin_dependency;

    gason::JsonValue jDep = json( "dependencies" );
    if( !jDep ) {
        return {};
    }

    gason::JsonValue jPlugins = jDep( "plugins" );
    if( !jPlugins ) {
        return {};
    }

    if( !jPlugins.isArray() ) {
        return {};
    }

    gason::JsonValue element;
    int index = 0;
    do {
        element = jPlugins[index++];
        if( element ) {
            plugin_dependency.emplace_back();
            plugin_dependency.back().id = element( "id" );
            if( 2 != sscanf( element( tokens::Version ).toString(),
                             "%hhu.%hhu",
                             &plugin_dependency.back().version.major,
                             &plugin_dependency.back().version.minor ) ) {
                FUNC_LOGE( "Fail parsing version " );
            }
        }
    } while( element );

    return plugin_dependency;
}

}  // namespace plugin
