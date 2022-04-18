#pragma once
#include <string>
#include <cstdint>

#include <extension_info.h>
#include <StorageFileFormat.h>

namespace plugin {

using PluginID = extension::ExtensionID;

using Version = extension::Version;

using PluginInfo = extension::ExtensionInfo<PluginID, Version>;

struct PluginURI {
    PluginInfo plugin;
    std::string uri;
    std::uint32_t size;
    storage::FileFormat format = storage::FileFormat::UNKNOWN;
};

}  // namespace plugin
