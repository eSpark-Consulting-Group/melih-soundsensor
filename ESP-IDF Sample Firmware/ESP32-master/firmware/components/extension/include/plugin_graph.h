#pragma once

#include <plugin_info.h>
#include <AdjucencyList.h>

namespace plugin {

using PluginGraph = common::AdjacencyList<PluginInfo>;

}  // namespace plugin
