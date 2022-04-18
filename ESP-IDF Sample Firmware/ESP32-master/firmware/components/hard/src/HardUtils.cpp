#include <HardUtils.h>
#include <esp_system.h>

namespace hard {
namespace common {

uint32_t generateUniqueId() {
    return esp_random();
}

}  // namespace common
}  // namespace hard
