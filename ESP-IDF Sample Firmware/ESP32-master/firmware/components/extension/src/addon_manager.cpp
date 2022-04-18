#include <addon_manager.h>
#include <esp_littlefs.h>
#include <lua.hpp>
#include <esp_system.h>

#include <cstdlib>
#include <cstdio>

namespace addon {

std::list<AddonInfo> AddonManager::getInstalledAddonList() {
    auto luaVerMajor = std::atoi( LUA_VERSION_MAJOR );
    auto luaVerMinor = std::atoi( LUA_VERSION_MINOR );

    const auto idfVer = esp_get_idf_version();
    unsigned int idfMinor = 0, idfMajor = 0;
    std::sscanf( idfVer, "v%u.%u", &idfMajor, &idfMinor );

    return { { "znet", 0, 1 },
             { "littlefs", LFS_VERSION_MAJOR, LFS_VERSION_MINOR },
             { "lua", std::uint8_t( luaVerMajor ), std::uint8_t( luaVerMinor ) },
             { "esp-idf", std::uint8_t( idfMajor ), std::uint8_t( idfMinor ) } };
}

}  // namespace addon
