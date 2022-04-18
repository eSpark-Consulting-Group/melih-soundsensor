#include <LedDevice.h>
#include <DebugUtils.h>
#include <Defs.h>

namespace hard {

LedDevice::LedDevice()
: m_initialized( false )
, m_ledcontrolType( LEDControlInit )
, m_source( ResetSource )
, m_flashTime( 0 )
, m_startTime( 0 )
, m_flashes( 0 ) {}

LedDevice& LedDevice::get() {
    static LedDevice ledcontrol;
    return ledcontrol;
}

void LedDevice::set( LEDControlType ledcontrolType ) {
    FUNC_LOGE( "Not implement" );
}

void LedDevice::setFlashMode( LEDControlSource source,
                              LEDControlType ledcontrolType,
                              bool flashed,
                              int time,
                              int8_t flashes ) {
    FUNC_LOGE( "Not implement" );
}

void LedDevice::check() {
    FUNC_LOGE( "Not implement" );
}
void LedDevice::setDefaultState() {
    m_flashTime = 0;
    m_startTime = 0;
    m_flashes = 0;
    m_source = ResetSource;
    m_ledcontrolType = LEDControlInit;
}

bool LedDevice::initialize() {
    FUNC_LOGE( "Not implement" );
    return true;
}

}  // namespace hard
