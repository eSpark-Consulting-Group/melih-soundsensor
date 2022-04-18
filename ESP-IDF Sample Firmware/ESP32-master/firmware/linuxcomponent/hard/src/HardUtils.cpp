#include <HardUtils.h>
#include <cstdlib>

namespace hard {
namespace common {

uint32_t generateUniqueId() {
    return std::rand();
}

}  // namespace common
}  // namespace hard
