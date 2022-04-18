#pragma once
#include <string>
#include <cstdint>

#include <extension_info.h>

namespace addon {

using AddonID = extension::ExtensionID;

using Version = extension::Version;

using AddonInfo = extension::ExtensionInfo<AddonID, Version>;

}  // namespace addon
