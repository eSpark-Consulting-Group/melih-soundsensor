#pragma once

#include <list>

#include <addon_info.h>

namespace addon {

class AddonManager {
public:
    static std::list<AddonInfo> getInstalledAddonList();
};

}  // namespace addon
