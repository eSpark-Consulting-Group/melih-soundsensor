#include <DebugUtils.h>
#include "Reset.h"

namespace hard {

Reset::Reset()
: m_initialized( false )
, m_resetType( ResetType::Undefined )
, m_frontTime( 0 )
, m_pressedTime( 0 )
, m_shorts( 0 ) {}

Reset& Reset::get() {
    static Reset reset;
    return reset;
}

bool Reset::initialize() {
    FUNC_LOGE( "Not implement" );
    return true;
}

void Reset::check() {
    FUNC_LOGE( "Not implement" );
}

void Reset::setCallbackBroadcast( resetCallback_t cb ) {
    m_broadcast = cb;
}

void Reset::setCallbackResetWifi( resetCallback_t cb ) {
    m_resetWifi = cb;
}

void Reset::setCallbackSoftReset( resetCallback_t cb ) {
    m_softReset = cb;
}

void Reset::setCallbackResetToFactoryDefaults( resetCallback_t cb ) {
    m_resetToFactoryDefaults = cb;
}

void Reset::setCallbackUseOtherFirmware( resetCallback_t cb ) {
    m_useOtherFirmware = cb;
}

void Reset::setDefaultState() {
    m_frontTime = 0;
    m_pressedTime = 0;
    m_frontTime = 0;
    m_shorts = 0;
    m_resetType = ResetType::Undefined;
}

void Reset::handleButtonPressed( int64_t time, bool pressed ) {
    FUNC_LOGE( "Not implement" );
}

void Reset::gpioCallback( void* arg ) {
    FUNC_LOGE( "Not implement" );
}

}  // namespace hard
