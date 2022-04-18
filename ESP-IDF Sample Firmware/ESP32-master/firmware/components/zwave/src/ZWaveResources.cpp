
#include "ZWaveResources.h"

namespace gateways {
namespace zwave {
namespace resources {
namespace storage {
namespace version {

const std::uint8_t Major = 7;
const std::uint8_t Minor = 0;
const std::uint8_t HeaderSize = sizeof( Major ) + sizeof( Minor );

}  // namespace version
}  // namespace storage

constexpr const char Tag[] = "ZWAVE";

}  // namespace resources
}  // namespace zwave
}  // namespace gateways
