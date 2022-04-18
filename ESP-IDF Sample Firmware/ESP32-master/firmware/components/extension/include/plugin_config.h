#pragma once

#include <list>
#include <vector>
#include <memory>

#include <plugin_info.h>
#include <addon_info.h>
#include <gason.hpp>

namespace plugin {

struct PluginConfig {
    PluginConfig();

    PluginConfig( PluginConfig&& other );

    PluginConfig( const PluginConfig& ) = delete;

    PluginConfig& operator=( PluginConfig&& other );

    PluginConfig& operator=( const PluginConfig& ) = delete;

    std::string getId();

    std::string getType();

    PluginInfo getPlugin();

    std::list<PluginInfo> getPluginDependency();

    gason::JsonValue json;

    std::unique_ptr<gason::JsonAllocator> jsonAlloc;

    std::vector<char> buff;
};

}  // namespace plugin
