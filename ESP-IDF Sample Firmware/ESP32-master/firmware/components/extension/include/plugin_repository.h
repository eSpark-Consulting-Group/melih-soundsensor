#pragma once
#include <string>
#include <list>

#include <plugin_info.h>

namespace plugin {

class PluginRepository {
public:
    PluginRepository( const std::string& serverURI );
    PluginURI getPluginURI( const PluginID& id, const Version& version ) const;
    // TODO: Get plugin list from repository including dependencies

private:
    std::string m_serverURI;
};

}  // namespace plugin
