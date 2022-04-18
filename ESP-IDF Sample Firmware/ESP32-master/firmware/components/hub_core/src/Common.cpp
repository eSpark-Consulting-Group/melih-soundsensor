#include "hub_core/Common.h"

#include "IDInfo.h"

namespace core {

const char* serial() {
    static constexpr size_t SerialWidth = 16;
    static char serialBuffer[SerialWidth + 1] = {};
    if( serialBuffer[0] == '\0' ) {
        snprintf( serialBuffer, sizeof( serialBuffer ), "%llu", core::IDInfo::get().id() );
    }
    return serialBuffer;
}

std::string hubName() {
#if CONFIG_BOARD_TYPE_PLUG_HUB
    return std::string( "PlugHub_" ) + core::serial();
#elif CONFIG_BOARD_TYPE_WATER_VALVE
    // return std::string( "WaterValve_" ) + core::serial();
    return std::string( "Atom_" ) + core::serial();
#else
    return std::string( "Atom_" ) + core::serial();
#endif
}

}  // namespace core