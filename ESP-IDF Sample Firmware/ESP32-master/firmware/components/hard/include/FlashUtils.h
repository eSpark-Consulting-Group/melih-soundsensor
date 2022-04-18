#include <cstdint>

namespace hard {

bool eraseFlash( std::size_t start, std::size_t size );

bool writeToFlash( std::size_t dest, const void* src, std::size_t size );

}  // namespace hard
